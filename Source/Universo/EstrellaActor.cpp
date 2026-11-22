// Copyright (c) 2026 Universo. All Rights Reserved.

#include "EstrellaActor.h"
#include "UGI_Espacio.h"
#include "SkillsSubsystem.h"
#include "EngineUtils.h"
#include "EvolutionComponent.h"

AEstrellaActor::AEstrellaActor()
{
    if (bCargadaDesdeSave) return;
    if (EtapaEvolucion >= 1) bBoton0Usado = true;

    PrimaryActorTick.bCanEverTick = true;
    MeshEstrella = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshEstrella"));
    RootComponent = MeshEstrella;
}

void AEstrellaActor::BeginPlay()
{
    Super::BeginPlay();

    // 🚀 ASIGNAR ID
    if (!bCargadaDesdeSave && IDEstrella <= 0)
    {
        if (UUGI_Espacio* GI = Cast<UUGI_Espacio>(GetGameInstance()))
        {
            IDEstrella = GI->ObtenerSiguienteIDEstrella();
        }
    }



    // 🌟 LÓGICA DE ECO CÓSMICO
    if (!bCargadaDesdeSave && !bEsEvolucion && IDEstrella > 1)
    {
        if (UUGI_Espacio* GI = Cast<UUGI_Espacio>(GetGameInstance()))
        {
            if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
            {
                if (Skills->EsEcoCosmicoActivo())
                {
                    AEstrellaActor* EstrellaAnterior = nullptr;
                    for (TActorIterator<AEstrellaActor> It(GetWorld()); It; ++It)
                    {
                        if (IsValid(*It) && It->IDEstrella == (IDEstrella - 1))
                        {
                            EstrellaAnterior = *It;
                            break;
                        }
                    }

                    if (EstrellaAnterior)
                    {
                        float Porcentaje = Skills->ObtenerPorcentajeEcoCosmico();
                        EstrellaAnterior->ActivarEcoCosmico(Porcentaje);
                    }
                }
            }
        }
    }

    // 🚀 AUTO-DESACTIVACIÓN DE BOTONES
    if (EtapaEvolucion >= 1) bBoton0Usado = true;
    if (EtapaEvolucion >= 2) bBoton1Usado = true;
    if (EtapaEvolucion >= 3) bBoton2Usado = true;
    if (EtapaEvolucion >= 4) bBoton3Usado = true;
    if (EtapaEvolucion >= 5) bBoton4Usado = true;
    if (EtapaEvolucion >= 6) bBoton5Usado = true;
    if (EtapaEvolucion >= 7) bBoton6Usado = true;
    if (EtapaEvolucion >= 8) bBoton7Usado = true;
    if (EtapaEvolucion >= 9) bBoton8Usado = true;
    if (EtapaEvolucion >= 10) bBoton9Usado = true;
    if (EtapaEvolucion >= 11) bBoton10Usado = true;

    // 👇 NUEVO: Si es nebulosa, arrancamos la generación pasiva
    if (bEsNebulosa)
    {
        GetWorld()->GetTimerManager().SetTimer(
            TimerNebulosa,
            this,
            &AEstrellaActor::GenerarPolvoPasivo,
            IntervaloPolvoPasivo,
            true
        );
    }
}

void AEstrellaActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TimerNebulosa);
    }
}

// 👇 NUEVA FUNCIÓN: Generar el polvo
void AEstrellaActor::GenerarPolvoPasivo()
{
    if (!bEsNebulosa) return;

    UUGI_Espacio* GI = Cast<UUGI_Espacio>(GetGameInstance());
    if (!GI) return;

    // Nivel de la nebulosa (si EtapaEvolucion es 0, contamos como nivel 1)
   
    int32 PolvoBase = PolvoBasePorNivel;

    // Aplicar multiplicador de la habilidad "Nebulosa Activa"
    float MultiplicadorSkill = 1.0f;
    if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
    {
        if (Skills->EsNebulosaActivaComprada())
        {
            float Porcentaje = Skills->ObtenerPorcentajePolvoNebulosa();
            MultiplicadorSkill = 1.0f + (Porcentaje / 100.0f);
        }
    }

    int32 PolvoFinal = FMath::RoundToInt(PolvoBase * MultiplicadorSkill);

    // Sumamos el polvo directo (sin multiplicadores de clic)
    GI->AddPolvo(PolvoFinal);
}


// 👇 TICK CORREGIDO (solo una vez, con las llaves bien puestas)
void AEstrellaActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bCinturonAsteroidesActivo && TiempoRestanteCinturon > 0.0f)
    {
        TiempoRestanteCinturon -= DeltaTime;
        if (TiempoRestanteCinturon <= 0.0f)
        {
            bCinturonAsteroidesActivo = false;
            MultiplicadorCinturon = 1.0f;
            UE_LOG(LogTemp, Log, TEXT("[Cinturon] Buff expirado en %s"), *GetName());
        }
    }
}

// 👇 FUNCIÓN DEL CINTURÓN CORREGIDA (fuera del Tick)
void AEstrellaActor::ActivarCinturonAsteroides(float Porcentaje, float Duracion)
{
    bCinturonAsteroidesActivo = true;
    MultiplicadorCinturon = 1.0f + (Porcentaje / 100.0f);
    TiempoRestanteCinturon = Duracion; // -1 => permanente
}


// 👇 FUNCIÓN DEL ECO CÓSMICO CORREGIDA (fuera del Tick)
void AEstrellaActor::ActivarEcoCosmico(float Porcentaje)
{
    bEcoCosmicoActivo = true;
    MultiplicadorEco = 1.0f + (Porcentaje / 100.0f);
}

void AEstrellaActor::EvolucionarAutomaticamente()
{
    // Verificamos que la estrella tenga configurada su siguiente evolución en el Blueprint
    if (!ClaseSiguiente)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Auto-Evo] La estrella %s no tiene ClaseSiguiente configurada."), *GetName());
        return;
    }

    UEvolutionComponent* EvoComp = FindComponentByClass<UEvolutionComponent>();

    // Nos aseguramos de que no esté ya evolucionando y que su animación inicial haya terminado
    if (EvoComp && bAnimacionInicialTerminada)
    {
        // Le decimos al componente cuál es la siguiente clase a spawnear
        EvoComp->SuccessorStarClass = ClaseSiguiente;

        // Aumentamos la etapa para que la UI y el guardado se actualicen
        EtapaEvolucion++;

        // Iniciamos la animación de encogimiento y posterior evolución
        EvoComp->IniciarEvolucionEnReversa();

        UE_LOG(LogTemp, Warning, TEXT("[Auto-Evo] ¡La estrella %s ha evolucionado automáticamente!"), *GetName());
    }
}