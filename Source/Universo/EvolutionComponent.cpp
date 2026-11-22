#include "EvolutionComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "BPC_GeneradorPolvo.h"
#include "NiagaraComponent.h"
#include "SpaceCameraPawn.h"
#include "EstrellaActor.h"
#include "Kismet/GameplayStatics.h"
#include "UGI_Espacio.h"

UEvolutionComponent::UEvolutionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    AnimState = EEvolutionAnimState::None;
}

void UEvolutionComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (AEstrellaActor* Estrella = Cast<AEstrellaActor>(Owner))
    {
        if (Estrella->bAnimacionInicialTerminada)
        {
            SetNiagaraScale(InitialScaleValue);
            AnimState = EEvolutionAnimState::None;

            // 👇 Arrancamos el generador desde C++ al cargar partida
            if (UBPC_GeneradorPolvo* Generador = Owner->FindComponentByClass<UBPC_GeneradorPolvo>())
            {
                Generador->IniciarGeneracionPolvo();
                UE_LOG(LogTemp, Log, TEXT("[Evolution] Generador arrancado tras carga (C++)."));
            }

            bPendienteEmitirSpawnFinished = true;
            return;
        }
    }

    IniciarSpawnAnimation();
}

void UEvolutionComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bPendienteEmitirSpawnFinished)
    {
        bPendienteEmitirSpawnFinished = false;
        OnSpawnFinished.Broadcast();
        UE_LOG(LogTemp, Log, TEXT("[Evolution] OnSpawnFinished emitido tras la carga."));
        return;
    }

    if (AnimState == EEvolutionAnimState::None)
    {
        return;
    }

    if (CurrentActiveDuration <= 0.0f)
    {
        if (AnimState == EEvolutionAnimState::Spawning)
        {
            AnimState = EEvolutionAnimState::None;
            SetNiagaraScale(CurrentTargetScaleForAnim);
            OnSpawnFinished.Broadcast();

            // 👇 NUEVO: Guardamos que la animación ya terminó (Primera vía)
            if (AEstrellaActor* Estrella = Cast<AEstrellaActor>(GetOwner()))
            {
                Estrella->bAnimacionInicialTerminada = true;
            }

            // 👇 NUEVO: Arrancar el generador de polvo desde C++ al nacer
            if (UBPC_GeneradorPolvo* Generador = GetOwner()->FindComponentByClass<UBPC_GeneradorPolvo>())
            {
                Generador->IniciarGeneracionPolvo();
            }
        }
        else if (AnimState == EEvolutionAnimState::Shrinking)
        {
            AnimState = EEvolutionAnimState::None;
            FinalizarEvolucion();
        }
        return;
    }

    CurrentAnimationTime += DeltaTime;

    float Progress = FMath::Clamp(
        CurrentAnimationTime / CurrentActiveDuration,
        0.0f,
        1.0f
    );

    float CurveValue = Progress;
    if (AnimationCurve)
    {
        CurveValue = AnimationCurve->GetFloatValue(Progress);
    }
    CurveValue = FMath::Clamp(CurveValue, 0.0f, 1.0f);

    float TargetScale = FMath::Lerp(
        CurrentStartScaleForAnim,
        CurrentTargetScaleForAnim,
        CurveValue
    );

    SetNiagaraScale(TargetScale);

    if (Progress >= 1.0f)
    {
        if (AnimState == EEvolutionAnimState::Spawning)
        {
            AnimState = EEvolutionAnimState::None;
            SetNiagaraScale(CurrentTargetScaleForAnim);
            OnSpawnFinished.Broadcast();

            // 👇 NUEVO: Guardamos que la animación ya terminó (Segunda vía)
            if (AEstrellaActor* Estrella = Cast<AEstrellaActor>(GetOwner()))
            {
                Estrella->bAnimacionInicialTerminada = true;
            }

            // 👇 NUEVO: Arrancar el generador de polvo desde C++ al nacer
            if (UBPC_GeneradorPolvo* Generador = GetOwner()->FindComponentByClass<UBPC_GeneradorPolvo>())
            {
                Generador->IniciarGeneracionPolvo();
            }
        }
        else if (AnimState == EEvolutionAnimState::Shrinking)
        {
            AnimState = EEvolutionAnimState::None;
            FinalizarEvolucion();
        }
    }
}

void UEvolutionComponent::IniciarSpawnAnimation()
{
    AnimState = EEvolutionAnimState::Spawning;
    CurrentAnimationTime = 0.0f;
    CurrentStartScaleForAnim = SpawnStartScale;
    CurrentTargetScaleForAnim = InitialScaleValue;
    CurrentActiveDuration = SpawnAnimationDuration;
    SetNiagaraScale(CurrentStartScaleForAnim);
}

void UEvolutionComponent::IniciarEvolucionEnReversa()
{
    if (AnimState == EEvolutionAnimState::Shrinking) return;

    AActor* Owner = GetOwner();
    if (Owner)
    {
        if (UBPC_GeneradorPolvo* GeneradorPolvo = Owner->FindComponentByClass<UBPC_GeneradorPolvo>())
        {
            // Detenemos la creación de polvo inmediatamente
            GeneradorPolvo->DetenerGeneracionPolvo();

            // Y ahora sí, retiramos los que ya existen
            GeneradorPolvo->RetirarTodoElPolvo();
        }
    }

    AnimState = EEvolutionAnimState::Shrinking;
    CurrentAnimationTime = 0.0f;
    CurrentStartScaleForAnim = InitialScaleValue;
    CurrentTargetScaleForAnim = FinalScaleValue;
    CurrentActiveDuration = AnimationDuration;
}

void UEvolutionComponent::SetNiagaraScale(float Scale)
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    UNiagaraComponent* NiagaraComp = Owner->FindComponentByClass<UNiagaraComponent>();
    if (NiagaraComp)
    {
        NiagaraComp->SetVariableFloat(NiagaraParamName, Scale);
    }
}

void UEvolutionComponent::FinalizarEvolucion()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    FTransform SpawnTransform = Owner->GetActorTransform();
    UWorld* World = GetWorld();

    if (World)
    {
        if (UUGI_Espacio* GI = Cast<UUGI_Espacio>(UGameplayStatics::GetGameInstance(World)))
        {
            GI->AvanzarNivelCamara();
            GI->IncrementarEstrellasEvolucionadas(1);
        }
    }

    // Aquí solo limpiamos los actores que sigan pegados (Niagara, etc.)
    TArray<AActor*> AttachedActors;
    Owner->GetAttachedActors(AttachedActors);
    for (AActor* Child : AttachedActors)
    {
        if (Child)
        {
            Child->Destroy();
        }
    }

    if (SuccessorStarClass)
    {
        AActor* NuevaEstrella = World->SpawnActor<AActor>(SuccessorStarClass, SpawnTransform);
        if (NuevaEstrella)
        {
            AEstrellaActor* ViejaEstrella = Cast<AEstrellaActor>(Owner);
            AEstrellaActor* NuevaEstrellaActor = Cast<AEstrellaActor>(NuevaEstrella);

            if (ViejaEstrella && NuevaEstrellaActor)
            {
                // 🚀 ASIGNAR EL LINAJE (PADRE E HIJO)
                NuevaEstrellaActor->ClaseAnterior = ViejaEstrella->GetClass();
                ViejaEstrella->ClaseSiguiente = NuevaEstrellaActor->GetClass();

                // Copiar el estado de los botones...
                NuevaEstrellaActor->bBoton0Usado = ViejaEstrella->bBoton0Usado;
                NuevaEstrellaActor->bBoton1Usado = ViejaEstrella->bBoton1Usado;
                NuevaEstrellaActor->bBoton2Usado = ViejaEstrella->bBoton2Usado;
                NuevaEstrellaActor->bBoton3Usado = ViejaEstrella->bBoton3Usado;
                NuevaEstrellaActor->bBoton4Usado = ViejaEstrella->bBoton4Usado;
                NuevaEstrellaActor->bBoton5Usado = ViejaEstrella->bBoton5Usado;
                NuevaEstrellaActor->bBoton6Usado = ViejaEstrella->bBoton6Usado;
                NuevaEstrellaActor->bBoton7Usado = ViejaEstrella->bBoton7Usado;
                NuevaEstrellaActor->bBoton8Usado = ViejaEstrella->bBoton8Usado;
                NuevaEstrellaActor->bBoton9Usado = ViejaEstrella->bBoton9Usado;
                NuevaEstrellaActor->bBoton10Usado = ViejaEstrella->bBoton10Usado;

               
            }

            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
            if (ASpaceCameraPawn* CameraPawn = Cast<ASpaceCameraPawn>(PlayerPawn))
            {
                if (CameraPawn->TargetActor == Owner)
                {
                    CameraPawn->FijarNuevaEstrellaEvolucionada(NuevaEstrella);
                }
            }
        }
    }

    // 4. Finalmente, destruimos la vieja estrella
    Owner->Destroy();
}