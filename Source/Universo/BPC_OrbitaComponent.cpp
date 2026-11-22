// Copyright (c) 2026 Universo. All Rights Reserved.

#include "BPC_OrbitaComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EngineUtils.h"

UBPC_OrbitaComponent::UBPC_OrbitaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBPC_OrbitaComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (Owner && Owner->GetRootComponent())
    {
        OrbitCollider = NewObject<USphereComponent>(Owner, TEXT("OrbitColliderDynamic"));
        if (OrbitCollider)
        {
            OrbitCollider->RegisterComponent();
            OrbitCollider->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            OrbitCollider->SetSphereRadius(RadioOrbitaBase);
            OrbitCollider->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
            OrbitCollider->SetGenerateOverlapEvents(true);
        }
    }
}

void UBPC_OrbitaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (OrbitCollider && OrbitCollider->GetUnscaledSphereRadius() != RadioOrbitaBase)
    {
        OrbitCollider->SetSphereRadius(RadioOrbitaBase);
    }

    AplicarRepulsionOrbital(DeltaTime);

    if (bMostrarOrbita)
    {
        DibujarOrbitaVisual();
    }
}

void UBPC_OrbitaComponent::AplicarRepulsionOrbital(float DeltaTime)
{
    if (!OrbitCollider) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (bEstaCreciendo) return;

    TArray<UPrimitiveComponent*> OverlappingComps;
    OrbitCollider->GetOverlappingComponents(OverlappingComps);

    for (UPrimitiveComponent* Comp : OverlappingComps)
    {
        USphereComponent* OtroCollider = Cast<USphereComponent>(Comp);
        if (!OtroCollider || OtroCollider == OrbitCollider) continue;

        AActor* OtroActor = OtroCollider->GetOwner();
        if (!OtroActor || OtroActor == Owner) continue;

        UBPC_OrbitaComponent* OtraOrbita = OtroActor->FindComponentByClass<UBPC_OrbitaComponent>();
        if (!OtraOrbita) continue;

        FVector DireccionEmpuje = Owner->GetActorLocation() - OtroActor->GetActorLocation();
        float DistanciaActual = DireccionEmpuje.Size();

        float RadioPropio = OrbitCollider->GetScaledSphereRadius();
        float RadioOtro = OtroCollider->GetScaledSphereRadius();
        float DistanciaMinima = RadioPropio + RadioOtro;

        if (DistanciaActual < DistanciaMinima && DistanciaActual > KINDA_SMALL_NUMBER)
        {
            DireccionEmpuje /= DistanciaActual;

            float Solapamiento = DistanciaMinima - DistanciaActual;
            float PasoEmpuje = FMath::Min(Solapamiento, FuerzaRepulsion * DeltaTime);
            FVector Desplazamiento = DireccionEmpuje * PasoEmpuje;

            Owner->AddActorWorldOffset(Desplazamiento, false);
        }
    }
}

void UBPC_OrbitaComponent::DibujarOrbitaVisual()
{
    if (!OrbitCollider) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    float RadioEfectivo = OrbitCollider->GetScaledSphereRadius();
    FVector Ubicacion = Owner->GetActorLocation();

    DrawDebugCircle(
        GetWorld(), Ubicacion, RadioEfectivo, 64, ColorOrbita,
        false, -1.0f, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0), false
    );

    DrawDebugSphere(
        GetWorld(), Ubicacion, RadioEfectivo, 16, ColorOrbita,
        false, -1.0f, 0, 1.0f
    );
}

float UBPC_OrbitaComponent::GetRadioOrbitaEfectivo() const
{
    if (OrbitCollider)
    {
        return OrbitCollider->GetScaledSphereRadius();
    }

    AActor* Owner = GetOwner();
    if (Owner)
    {
        return RadioOrbitaBase * Owner->GetActorScale3D().GetMax();
    }

    return RadioOrbitaBase;
}

float UBPC_OrbitaComponent::GetRadioOrbitaDeClase(TSubclassOf<AActor> ActorClass)
{
    if (!ActorClass) return 0.0f;

    UClass* CurrentClass = ActorClass;
    while (CurrentClass && CurrentClass != AActor::StaticClass())
    {
        if (UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(CurrentClass))
        {
            for (UActorComponent* Template : BPClass->ComponentTemplates)
            {
                if (UBPC_OrbitaComponent* OrbitaComp = Cast<UBPC_OrbitaComponent>(Template))
                {
                    return OrbitaComp->RadioOrbitaBase;
                }
            }

            if (BPClass->SimpleConstructionScript)
            {
                const TArray<USCS_Node*>& Nodes = BPClass->SimpleConstructionScript->GetAllNodes();
                for (USCS_Node* Node : Nodes)
                {
                    if (Node && Node->ComponentTemplate && Node->ComponentTemplate->IsA<UBPC_OrbitaComponent>())
                    {
                        if (UBPC_OrbitaComponent* OrbitaTemplate = Cast<UBPC_OrbitaComponent>(Node->ComponentTemplate))
                        {
                            return OrbitaTemplate->RadioOrbitaBase;
                        }
                    }
                }
            }
        }
        CurrentClass = CurrentClass->GetSuperClass();
    }

    AActor* DefaultActor = ActorClass->GetDefaultObject<AActor>();
    if (DefaultActor)
    {
        UBPC_OrbitaComponent* Comp = DefaultActor->FindComponentByClass<UBPC_OrbitaComponent>();
        if (Comp)
        {
            return Comp->RadioOrbitaBase;
        }
    }

    return 0.0f;
}

bool UBPC_OrbitaComponent::EncontrarPosicionOrbitalValida(
    UObject* WorldContextObject,
    float MinRadius,
    float MaxRadius,
    float MinSeparation,
    int32 MaxAttempts,
    FVector& OutLocation)
{
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) return false;

    TArray<FVector> PosicionesExistentes;
    TArray<float>   RadiosExistentes;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor)) continue;

        UBPC_OrbitaComponent* Orbita = Actor->FindComponentByClass<UBPC_OrbitaComponent>();
        if (Orbita)
        {
            PosicionesExistentes.Add(Actor->GetActorLocation());
            RadiosExistentes.Add(Orbita->GetRadioOrbitaEfectivo());
        }
    }

    for (int32 Intento = 0; Intento < MaxAttempts; ++Intento)
    {
        const float Angulo = FMath::FRandRange(0.0f, 360.0f);
        const float Distancia = FMath::FRandRange(MinRadius, MaxRadius);
        const float Radianes = FMath::DegreesToRadians(Angulo);

        FVector Candidata;
        Candidata.X = FMath::Cos(Radianes) * Distancia;
        Candidata.Y = FMath::Sin(Radianes) * Distancia;
        Candidata.Z = FMath::FRandRange(-1000.0f, 1000.0f);

        bool bValida = true;
        for (int32 i = 0; i < PosicionesExistentes.Num(); ++i)
        {
            const float DistanciaMinima = MinSeparation + RadiosExistentes[i];
            if (FVector::Dist(Candidata, PosicionesExistentes[i]) < DistanciaMinima)
            {
                bValida = false;
                break;
            }
        }

        if (bValida)
        {
            OutLocation = Candidata;
            return true;
        }
    }

    OutLocation = FVector::ZeroVector;
    return false;
}

void UBPC_OrbitaComponent::RegistrarEnSistemaOrbital()
{
}