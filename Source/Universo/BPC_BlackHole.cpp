// Copyright (c) 2026 Universo. All Rights Reserved.

#include "BPC_BlackHole.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "UGI_Espacio.h"
#include "BPC_GeneradorPolvo.h"
#include "SkillsSubsystem.h"
#include "UBPC_EfectoPolvo.h"

UBPC_BlackHole::UBPC_BlackHole()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBPC_BlackHole::BeginPlay()
{
    Super::BeginPlay();
}

void UBPC_BlackHole::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* Owner = GetOwner();
    if (!Owner) return;

    UWorld* World = GetWorld();
    if (!World) return;

    FVector CentroAgujeroNegro = Owner->GetActorLocation();

    // 1. DIBUJAR EL RADIO DE ATRACCIÓN
    if (bMostrarRadio)
    {
        DrawDebugSphere(
            World,
            CentroAgujeroNegro,
            RadioAtraccion,
            32,
            ColorRadio,
            false,
            -1.0f,
            0,
            2.0f
        );
    }

    // 2. BUSCAR TODO EL POLVO EN EL MAPA
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* PolvoActor = *It;
        if (!IsValid(PolvoActor) || PolvoActor == Owner) continue;

        // Verificamos si es polvo (por nombre o por tag)
        const bool bEsPolvo = PolvoActor->GetName().Contains(TEXT("Polvo")) ||
            PolvoActor->ActorHasTag(TEXT("PolvoEstelar"));

        if (!bEsPolvo) continue;

        // 3. CALCULAR DISTANCIA AL AGUJERO NEGRO
        FVector UbicacionPolvo = PolvoActor->GetActorLocation();
        float Distancia = FVector::Dist(UbicacionPolvo, CentroAgujeroNegro);

        if (Distancia > RadioAtraccion) continue;

        UBPC_EfectoPolvo* Efecto = PolvoActor->FindComponentByClass<UBPC_EfectoPolvo>();
        const bool bYaAbsorbido = Efecto && Efecto->EstaAbsorbido();

        // 4. ABSORCIÓN (se cobra UNA sola vez y se inicia la animación de salida)
        if (Distancia <= DistanciaAbsorcion && !bYaAbsorbido)
        {
            const int32 ValorPolvo = ObtenerValorPolvo(PolvoActor);
            if (ValorPolvo > 0)
            {
                if (UUGI_Espacio* GI = Cast<UUGI_Espacio>(World->GetGameInstance()))
                {
                    GI->AddPolvoConSkills(ValorPolvo);
                }
            }

            if (Efecto)
            {
                Efecto->IniciarAbsorcion(DuracionAnimacionAbsorcion);
            }
            else
            {
                PolvoActor->Destroy(); // sin componente de efecto no hay animación
                continue;
            }
        }

        // 5. ATRACCIÓN (también mientras se encoge, para que siga cayendo al centro)
        PolvoActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

        if (Efecto)
        {
            Efecto->bAtraidoPorAgujeroNegro = true;
        }

        const float Cercania = 1.0f - FMath::Clamp(Distancia / RadioAtraccion, 0.0f, 1.0f);
        const float Velocidad = FMath::Lerp(VelocidadAtraccionMinima, VelocidadAtraccionMaxima, Cercania * Cercania);

        PolvoActor->SetActorLocation(
            FMath::VInterpConstantTo(UbicacionPolvo, CentroAgujeroNegro, DeltaTime, Velocidad));
    }
}


int32 UBPC_BlackHole::ObtenerValorPolvo(AActor* PolvoActor) const
{
    if (!IsValid(PolvoActor)) return 0;

    // 1. Intentar obtener el valor desde el Generador de su dueño
    if (AActor* OwnerEstrella = PolvoActor->GetOwner())
    {
        if (UBPC_GeneradorPolvo* Generador = OwnerEstrella->FindComponentByClass<UBPC_GeneradorPolvo>())
        {
            return Generador->ObtenerValorDePolvo(PolvoActor);
        }
    }

    // 2. Fallback: Buscar si el actor de polvo tiene una propiedad "ValorPolvo" (Por si acaso)
    if (FIntProperty* Prop = CastField<FIntProperty>(PolvoActor->GetClass()->FindPropertyByName(TEXT("ValorPolvo"))))
    {
        return Prop->GetPropertyValue_InContainer(PolvoActor);
    }

    return 1; // Valor por defecto por si falla todo
}