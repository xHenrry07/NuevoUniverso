#include "BPC_CinturonAsteroides.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UGI_Espacio.h"
#include "SkillsSubsystem.h"
#include "BPC_GeneradorPolvo.h"
#include "EstrellaActor.h"
#include "UBPC_EfectoPolvo.h"

UBPC_CinturonAsteroides::UBPC_CinturonAsteroides()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetComponentTickEnabled(false);
}

void UBPC_CinturonAsteroides::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoActivar)
    {
        ActivarCinturon();
    }
}

void UBPC_CinturonAsteroides::ActivarCinturon()
{
    if (bActivo) return;

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;

    UUGI_Espacio* GI = Cast<UUGI_Espacio>(World->GetGameInstance());
    if (!GI) return;
    USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>();
    if (!Skills || !Skills->EsCinturonAsteroidesActivo()) return;

    bActivo = true;

    if (ClaseVisualCinturon)
    {
        FActorSpawnParameters Params;
        Params.Owner = Owner;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        VisualSpawned = World->SpawnActor<AActor>(
            ClaseVisualCinturon,
            Owner->GetActorTransform(),
            Params
        );

        if (VisualSpawned)
        {
            VisualSpawned->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
        }
    }

    if (AEstrellaActor* Estrella = Cast<AEstrellaActor>(Owner))
    {
        const float Porcentaje = Skills->ObtenerPorcentajeCinturonAsteroides();
        Estrella->ActivarCinturonAsteroides(Porcentaje, -1.0f);
    }

    SetComponentTickEnabled(true);

    UE_LOG(LogTemp, Warning, TEXT("[Cinturon] Activado en %s (radio=%.1f)"),
        *Owner->GetName(), RadioAtraccion);
}

void UBPC_CinturonAsteroides::DesactivarCinturon()
{
    if (!bActivo) return;
    bActivo = false;

    SetComponentTickEnabled(false);

    if (IsValid(VisualSpawned))
    {
        VisualSpawned->Destroy();
        VisualSpawned = nullptr;
    }

    if (AEstrellaActor* Estrella = Cast<AEstrellaActor>(GetOwner()))
    {
        Estrella->bCinturonAsteroidesActivo = false;
        Estrella->MultiplicadorCinturon = 1.0f;
        Estrella->TiempoRestanteCinturon = 0.0f;
    }
}

void UBPC_CinturonAsteroides::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bActivo) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    FVector CentroEstrella = Owner->GetActorLocation();

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Polvo = *It;
        if (!IsValid(Polvo) || Polvo == Owner) continue;

        if (Polvo->ActorHasTag(TEXT("PolvoEstelar")) || Polvo->GetName().Contains(TEXT("Polvo")))
        {
            FVector UbicacionPolvo = Polvo->GetActorLocation();

            // 1. Calcular la dirección desde el centro hacia el polvo
            FVector DireccionDesdeCentro = UbicacionPolvo - CentroEstrella;

            // 👇 NUEVO: Aplanamos la dirección (ignoramos la altura Z)
            DireccionDesdeCentro.Z = 0.0f;

            // Si el polvo está justo arriba o abajo de la estrella (centro del eje Z), 
            // le damos una dirección aleatoria en el plano XY para que empiece a orbitar
            if (DireccionDesdeCentro.IsNearlyZero())
            {
                DireccionDesdeCentro = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f);
            }
            DireccionDesdeCentro.Normalize();

            // 2. Calcular el punto exacto en el anillo (la geometría) - Ahora siempre estará al mismo nivel (Z) que la estrella
            FVector PuntoEnElCinturon = CentroEstrella + (DireccionDesdeCentro * RadioCinturon);

            // 3. Calcular distancia desde el polvo a ese punto del anillo
            float DistanciaAlCinturon = FVector::Dist(UbicacionPolvo, PuntoEnElCinturon);

            // Si el polvo está demasiado lejos del anillo, lo ignoramos
            if (DistanciaAlCinturon > RadioAtraccion) continue;

            // 4. ABSORCIÓN: Si el polvo toca el anillo, lo destruimos y damos el polvo
            if (DistanciaAlCinturon < DistanciaAbsorcion)
            {
                int32 Valor = ObtenerValorPolvo(Polvo);

                if (UUGI_Espacio* GI = Cast<UUGI_Espacio>(GetWorld()->GetGameInstance()))
                {
                    if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
                    {
                        const float Porcentaje = Skills->ObtenerPorcentajeCinturonAsteroides();
                        Valor = FMath::RoundToInt(Valor * (1.0f + (Porcentaje / 100.0f)));
                    }
                    GI->AddPolvoPasivo(Valor);
                }

                Polvo->Destroy();
            }
            // 5. ATRACCIÓN: Mover el polvo HACIA el anillo
            else
            {
                // 👇 AÑADE ESTA LÍNEA AQUÍ PARA DESPEGARLO DE LA ESTRELLA
                Polvo->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

                // La dirección de atracción va desde el polvo hacia el punto del anillo
                FVector DireccionAtraccion = (PuntoEnElCinturon - UbicacionPolvo).GetSafeNormal();

                Polvo->SetActorLocation(UbicacionPolvo + (DireccionAtraccion * VelocidadAtraccion * DeltaTime));

                if (UBPC_EfectoPolvo* Efecto = Polvo->FindComponentByClass<UBPC_EfectoPolvo>())
                {
                    Efecto->bAtraidoPorAgujeroNegro = true;
                }
            }
        }
    }
}

int32 UBPC_CinturonAsteroides::ObtenerValorPolvo(AActor* PolvoActor) const
{
    if (AActor* OwnerEstrella = PolvoActor->GetOwner())
    {
        if (UBPC_GeneradorPolvo* Generador = OwnerEstrella->FindComponentByClass<UBPC_GeneradorPolvo>())
        {
            return Generador->ObtenerValorDePolvo(PolvoActor);
        }
    }
    return 1;
}