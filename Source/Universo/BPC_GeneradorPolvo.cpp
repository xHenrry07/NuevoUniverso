#include "BPC_GeneradorPolvo.h"
#include "UBPC_EfectoPolvo.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "UGI_Espacio.h"
#include "SkillsSubsystem.h"
#include "EstrellaActor.h"

UBPC_GeneradorPolvo::UBPC_GeneradorPolvo()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UBPC_GeneradorPolvo::BeginPlay()
{
    Super::BeginPlay();

    OwnerActor = GetOwner();

    if (UWorld* World = GetWorld())
    {
        if (USkillsSubsystem* Skills = World->GetGameInstance()->GetSubsystem<USkillsSubsystem>())
        {
            if (Skills->EsOrbitaEstableActiva())
            {
                float Porcentaje = Skills->ObtenerPorcentajeOrbitaEstable();
                float Multiplicador = 1.0f + (Porcentaje / 100.0f);
                MaxPolvo = FMath::RoundToInt(MaxPolvo * Multiplicador);
            }
        }
    }

    if (bAutoStart)
    {
        IniciarGeneracionPolvo();
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
            {
                Skills->OnSkillComprada.AddDynamic(this, &UBPC_GeneradorPolvo::OnSkillCompradaRecibida);
            }
        }
    }

    VerificarMagnetismoEstelar();
    VerificarSupernovaDormida();
}

void UBPC_GeneradorPolvo::IniciarGeneracionPolvo()
{
    if (!OwnerActor || !ClasePolvoEstelar) return;
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_Spawn)) return;

    const float PrimerDelay = FMath::FRandRange(MinSpawnTime, MaxSpawnTime);
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle_Spawn, this, &UBPC_GeneradorPolvo::EjecutarCicloSpawn, PrimerDelay, false);
}

void UBPC_GeneradorPolvo::EjecutarCicloSpawn()
{
    if (PolvoActual < MaxPolvo)
    {
        int32 Intentos = 0;
        const int32 MaxIntentos = 20;
        bool bSpawnExitoso = false;

        while (Intentos < MaxIntentos && !bSpawnExitoso)
        {
            CalcularUbicacionCandidata();
            if (ValidarPosicion())
            {
                SpawnearPolvo();
                bSpawnExitoso = true;
            }
            Intentos++;
        }
    }
    ProgramarSiguienteSpawn();
}

void UBPC_GeneradorPolvo::CalcularUbicacionCandidata()
{
    if (!OwnerActor) return;
    const FVector DireccionAleatoria = UKismetMathLibrary::RandomUnitVector();
    const float DistanciaAleatoria = FMath::FRandRange(MinRadius, MaxRadius);
    const FVector OffsetLocal = DireccionAleatoria * DistanciaAleatoria;
    const FRotator RotacionAleatoria = UKismetMathLibrary::RandomRotator(false);
    const float EscalaAleatoria = FMath::FRandRange(MinScale, MaxScale);
    const FVector EscalaVector(EscalaAleatoria, EscalaAleatoria, EscalaAleatoria);
    UbicacionCandidata = FTransform(RotacionAleatoria, OffsetLocal, EscalaVector);
}

bool UBPC_GeneradorPolvo::ValidarPosicion()
{
    if (!OwnerActor) return false;
    const FVector CandidataLocal = UbicacionCandidata.GetLocation();
    const FTransform OwnerTransform = OwnerActor->GetActorTransform();

    for (const AActor* Polvo : PolvosCreados)
    {
        if (!IsValid(Polvo)) continue;
        const FVector PolvoLocal = OwnerTransform.InverseTransformPosition(Polvo->GetActorLocation());
        if (FVector::Dist(CandidataLocal, PolvoLocal) < MinDistanciaPolvo) return false;
    }
    return true;
}

void UBPC_GeneradorPolvo::SpawnearPolvo()
{
    UWorld* World = GetWorld();
    if (!World || !ClasePolvoEstelar || !OwnerActor || !OwnerActor->GetRootComponent()) return;

    int32 ValorBaseAjustado = ValorPolvo;
    if (UUGI_Espacio* GI = Cast<UUGI_Espacio>(World->GetGameInstance()))
    {
        if (GI->EsEstrellaAntigua(OwnerActor))
        {
            if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
            {
                float Porcentaje = Skills->ObtenerPorcentajeNucleoCaliente();
                float Multiplicador = 1.0f + (Porcentaje / 100.0f);
                ValorBaseAjustado = FMath::RoundToInt(ValorBaseAjustado * Multiplicador);
            }
        }
    }

    
    TSubclassOf<AActor> ClaseAUsar = ClasePolvoEstelar;
    int32 ValorFinalPolvo = ValorBaseAjustado;
    bool bEsEspecial = false;

    if (bSupernovaActiva)
    {
        bEsEspecial = true;
        ValorFinalPolvo = ValorBaseAjustado * MultiplicadorValorPolvoFino;
        if (ClasePolvoFino) ClaseAUsar = ClasePolvoFino;
        UE_LOG(LogTemp, Warning, TEXT("[Supernova] ¡Spawn de Polvo Dorado (forzado)! Valor: %d"), ValorFinalPolvo);
    }
    else if (bPermitePolvoFino)
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (USkillsSubsystem* Skills = GameInstance->GetSubsystem<USkillsSubsystem>())
            {
                if (Skills->EsPolvoFinoActivo())
                {
                    int32 Probabilidad = Skills->ObtenerProbabilidadPolvoFino();

                    // 👇 NUEVO: Añadir bonus por nebulosas si la skill está comprada
                    if (Skills->EsNebulosaActivaComprada())
                    {
                        if (UUGI_Espacio* UGI = Cast<UUGI_Espacio>(World->GetGameInstance()))
                        {
                            int32 CantNebulosas = UGI->ObtenerCantidadNebulosas();
                            float BonusPorNebulosa = Skills->ObtenerPorcentajePolvoFinoNebulosa();

                            Probabilidad += FMath::RoundToInt(CantNebulosas * BonusPorNebulosa);
                            Probabilidad = FMath::Clamp(Probabilidad, 0, 100); // Evitar pasar del 100%
                        }
                    }

                    int32 TiradaRandom = FMath::RandRange(1, 100);

                    if (TiradaRandom <= Probabilidad)
                    {
                        bEsEspecial = true;
                        ValorFinalPolvo = ValorBaseAjustado * MultiplicadorValorPolvoFino;
                        if (ClasePolvoFino) ClaseAUsar = ClasePolvoFino;
                        UE_LOG(LogTemp, Warning, TEXT("[Polvo Fino] ¡Spawn de Polvo Fino! Valor: %d"), ValorFinalPolvo);
                    }
                }
            }
        }
    } // <--- ESTA LLAVE ES LA QUE FALTABA O ESTABA MAL PUESTA

    const FVector OwnerLocation = OwnerActor->GetActorLocation();
    const FRotator OwnerRotation = OwnerActor->GetActorRotation();
    const FVector OffsetLocal = UbicacionCandidata.GetLocation();
    const FRotator RotacionLocal = UbicacionCandidata.GetRotation().Rotator();
    const FVector WorldLocation = OwnerLocation + OwnerRotation.RotateVector(OffsetLocal);
    const FRotator WorldRotation = OwnerRotation + RotacionLocal;
    const FVector WorldScale = UbicacionCandidata.GetScale3D();
    const FTransform WorldTransform(WorldRotation, WorldLocation, WorldScale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerActor;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* NuevoPolvo = World->SpawnActor<AActor>(ClaseAUsar, WorldTransform, SpawnParams);
    if (!NuevoPolvo) return;

    ValoresPolvoPersonalizados.Add(NuevoPolvo, ValorFinalPolvo);

    if (USceneComponent* PolvoRoot = NuevoPolvo->GetRootComponent())
    {
        PolvoRoot->SetMobility(EComponentMobility::Movable);
        PolvoRoot->AttachToComponent(OwnerActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
    }

    if (UBPC_EfectoPolvo* EfectoPolvo = NuevoPolvo->FindComponentByClass<UBPC_EfectoPolvo>())
    {
        EfectoPolvo->IniciarEfecto(UbicacionCandidata.GetScale3D());
    }

    NuevoPolvo->OnDestroyed.AddDynamic(this, &UBPC_GeneradorPolvo::AlDestruirPolvo);
    PolvosCreados.Add(NuevoPolvo);
    PolvoActual = PolvosCreados.Num();
}

void UBPC_GeneradorPolvo::ProgramarSiguienteSpawn()
{
    if (UWorld* World = GetWorld())
    {
        float SiguienteDelay = FMath::FRandRange(MinSpawnTime, MaxSpawnTime);
        if (AEstrellaActor* EstrellaOwner = Cast<AEstrellaActor>(OwnerActor))
        {
            if (EstrellaOwner->bEcoCosmicoActivo && EstrellaOwner->MultiplicadorEco > 1.0f)
            {
                SiguienteDelay /= EstrellaOwner->MultiplicadorEco;
            }
        }
        World->GetTimerManager().SetTimer(
            TimerHandle_Spawn, this, &UBPC_GeneradorPolvo::EjecutarCicloSpawn, SiguienteDelay, false);
    }
}

void UBPC_GeneradorPolvo::AlDestruirPolvo(AActor* DestroyedActor)
{
    if (!DestroyedActor) return;
    PolvosCreados.Remove(DestroyedActor);
    PolvoActual = PolvosCreados.Num();
    OnPolvoDestruido.Broadcast(DestroyedActor);
}

void UBPC_GeneradorPolvo::RetirarTodoElPolvo()
{
    for (AActor* Polvo : PolvosCreados)
    {
        if (IsValid(Polvo))
        {
            Polvo->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            if (UBPC_EfectoPolvo* Efecto = Polvo->FindComponentByClass<UBPC_EfectoPolvo>())
            {
                Efecto->IniciarRetiro(DuracionRetiroPolvo);
            }
        }
    }
}

void UBPC_GeneradorPolvo::DetenerGeneracionPolvo()
{
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_Spawn))
    {
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Spawn);
    }
}

void UBPC_GeneradorPolvo::RegistrarPolvoExistente(AActor* Polvo)
{
    if (!IsValid(Polvo)) return;
    if (PolvosCreados.Contains(Polvo)) return;

    PolvosCreados.Add(Polvo);
    PolvoActual = PolvosCreados.Num();

    if (!Polvo->OnDestroyed.IsAlreadyBound(this, &UBPC_GeneradorPolvo::AlDestruirPolvo))
    {
        Polvo->OnDestroyed.AddDynamic(this, &UBPC_GeneradorPolvo::AlDestruirPolvo);
    }

    UE_LOG(LogTemp, Log, TEXT("[Generador] Polvo registrado: %s (total=%d)"), *Polvo->GetName(), PolvoActual);
}

int32 UBPC_GeneradorPolvo::ObtenerValorDePolvo(AActor* Polvo) const
{
    if (!IsValid(Polvo)) return ValorPolvo;
    if (const int32* Encontrado = ValoresPolvoPersonalizados.Find(Polvo)) return *Encontrado;
    return ValorPolvo;
}

void UBPC_GeneradorPolvo::RegistrarValorPolvo(AActor* Polvo, int32 Valor)
{
    if (IsValid(Polvo)) ValoresPolvoPersonalizados.Add(Polvo, Valor);
}

void UBPC_GeneradorPolvo::OnSkillCompradaRecibida(FName IdSkill)
{
    if (IdSkill == FName("MagnetismoEstelar")) { VerificarMagnetismoEstelar(); }
    else if (IdSkill == FName("SupernovaDormida")) { VerificarSupernovaDormida(); }
    else if (IdSkill == FName("OrbitaEstable"))
    {
        if (UWorld* World = GetWorld())
        {
            if (USkillsSubsystem* Skills = World->GetGameInstance()->GetSubsystem<USkillsSubsystem>())
            {
                if (Skills->EsOrbitaEstableActiva())
                {
                    float Porcentaje = Skills->ObtenerPorcentajeOrbitaEstable();
                    float Multiplicador = 1.0f + (Porcentaje / 100.0f);
                    MaxPolvo = FMath::RoundToInt(MaxPolvo * Multiplicador);
                }
            }
        }
    }
}

void UBPC_GeneradorPolvo::VerificarMagnetismoEstelar()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (UGameInstance* GI = World->GetGameInstance())
    {
        if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
        {
            if (Skills->EsMagnetismoEstelarActivo())
            {
                float Intervalo = Skills->ObtenerIntervaloSkill(FName("MagnetismoEstelar"));
                if (Intervalo <= 0.0f) Intervalo = 3.0f;
                if (!World->GetTimerManager().IsTimerActive(TimerHandle_Magnetismo))
                {
                    World->GetTimerManager().SetTimer(
                        TimerHandle_Magnetismo, this, &UBPC_GeneradorPolvo::EjecutarMagnetismoEstelar, Intervalo, true);
                }
            }
        }
    }
}

void UBPC_GeneradorPolvo::EjecutarMagnetismoEstelar()
{
    if (PolvosCreados.Num() == 0) return;
    AActor* PolvoARecoger = nullptr;
    for (AActor* Polvo : PolvosCreados)
    {
        if (IsValid(Polvo)) { PolvoARecoger = Polvo; break; }
    }
    if (!PolvoARecoger) return;

    if (UWorld* World = GetWorld())
    {
        if (UUGI_Espacio* GI = Cast<UUGI_Espacio>(World->GetGameInstance()))
        {
            GI->AddPolvoConSkills(ObtenerValorDePolvo(PolvoARecoger));
        }
    }

    PolvoARecoger->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    if (UBPC_EfectoPolvo* Efecto = PolvoARecoger->FindComponentByClass<UBPC_EfectoPolvo>())
    {
        Efecto->IniciarRetiro(0.5f);
    }
    else
    {
        PolvoARecoger->Destroy();
    }
}

void UBPC_GeneradorPolvo::VerificarSupernovaDormida()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (UGameInstance* GI = World->GetGameInstance())
    {
        if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
        {
            if (Skills->EsSupernovaDormidaActiva())
            {
                float Intervalo = Skills->ObtenerIntervaloSupernova();
                if (Intervalo <= 0.0f) Intervalo = 60.0f;
                if (!World->GetTimerManager().IsTimerActive(TimerHandle_SupernovaIntervalo))
                {
                    World->GetTimerManager().SetTimer(
                        TimerHandle_SupernovaIntervalo, this, &UBPC_GeneradorPolvo::IntentarActivarSupernova, Intervalo, true);
                }
            }
        }
    }
}

void UBPC_GeneradorPolvo::IntentarActivarSupernova()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (UGameInstance* GI = World->GetGameInstance())
    {
        if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
        {
            const int32 Probabilidad = Skills->ObtenerProbabilidadSupernova();
            if (FMath::RandRange(1, 100) <= Probabilidad)
            {
                float Duracion = Skills->ObtenerDuracionSupernova();
                if (Duracion <= 0.0f) Duracion = 10.0f;
                ActivarSupernova(Duracion);
            }
        }
    }
}

void UBPC_GeneradorPolvo::ActivarSupernova(float Duracion)
{
    bSupernovaActiva = true;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TimerHandle_SupernovaDuracion, this, &UBPC_GeneradorPolvo::DesactivarSupernova, Duracion, false);
    }
    UE_LOG(LogTemp, Warning, TEXT("[Supernova] ACTIVADA durante %.1f segundos."), Duracion);
}

void UBPC_GeneradorPolvo::DesactivarSupernova()
{
    bSupernovaActiva = false;
    UE_LOG(LogTemp, Log, TEXT("[Supernova] DESACTIVADA."));
}