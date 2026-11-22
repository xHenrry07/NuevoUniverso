// Copyright (c) 2026 Universo. All Rights Reserved.

#include "UGI_Espacio.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGameUniverso.h"
#include "EstrellaActor.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "SpaceCameraPawn.h"
#include "UBPC_EfectoPolvo.h"
#include "GameFramework/SpringArmComponent.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"        // para TActorIterator
#include "BPC_GeneradorPolvo.h" // para RegistrarPolvoExistente
#include "SkillsSubsystem.h"
#include "BPC_OrbitaComponent.h"
#include "BPC_CinturonAsteroides.h"


UUGI_Espacio::UUGI_Espacio()
{
}

void UUGI_Espacio::Init()
{
    Super::Init();

    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        Skills->OnSkillComprada.AddDynamic(this, &UUGI_Espacio::OnSkillCompradaRecibida);
    }

    // Validación segura del World para evitar crash en Init()
    UWorld* World = GetWorld();
    if (World && World->IsGameWorld())
    {
        World->GetTimerManager().SetTimer(
            TimerAutoguardado,
            this,
            &UUGI_Espacio::AutoguardadoTick,
            TiempoAutoguardado,
            true
        );
    }
}

// =======================================================================
// SKILL: LLUVIA DE POLVO
// =======================================================================
void UUGI_Espacio::InicializarLluviaDePolvoSiCorresponde()
{
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        if (Skills->EstaSkillComprada(FName("LluviaDePolvo")))
        {
            UWorld* World = GetWorld();
            if (World && !World->GetTimerManager().IsTimerActive(TimerIntervaloLluvia))
            {
                // Segundos configurados en la DataTable
                float Intervalo = Skills->ObtenerIntervaloSkill(FName("LluviaDePolvo"));
                if (Intervalo <= 0.0f) Intervalo = 60.0f;

                World->GetTimerManager().SetTimer(
                    TimerIntervaloLluvia,
                    this,
                    &UUGI_Espacio::EjecutarLluviaDePolvo,
                    Intervalo,
                    true
                );
                UE_LOG(LogTemp, Log, TEXT("[Lluvia] Ciclo de Lluvia de Polvo iniciado cada %.1f segundos."), Intervalo);
            }
        }
    }
}

void UUGI_Espacio::EjecutarLluviaDePolvo()
{
    USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>();
    if (!Skills) return;

    const int32 CantidadX = Skills->ObtenerCantidadSkill(FName("LluviaDePolvo"));

    if (CantidadX > 0)
    {
        AddPolvo(CantidadX);
        OnLluviaDisparada.Broadcast(CantidadX);

        // 👇 NUEVO: Spawnear el efecto visual 3D
        if (ClaseVisualLluvia)
        {
            UWorld* World = GetWorld();
            if (World)
            {
                // Lo spawneamos arriba del jugador para que caiga sobre él
                if (APawn* Jugador = World->GetFirstPlayerController()->GetPawn())
                {
                    FVector UbicacionLluvia = Jugador->GetActorLocation() + FVector(0.f, 0.f, 1000.f);
                    FActorSpawnParameters Params;
                    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                    World->SpawnActor<AActor>(ClaseVisualLluvia, UbicacionLluvia, FRotator::ZeroRotator, Params);
                }
            }
        }

        UE_LOG(LogTemp, Warning, TEXT("[Lluvia] ¡Lluvia caída! Se otorgaron exactamente %d de polvo estelar."), CantidadX);
    }
}

// =======================================================================
// SKILL: CONSTELACIÓN I
// =======================================================================
void UUGI_Espacio::InicializarConstelacionISiCorresponde()
{
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        if (Skills->EstaSkillComprada(FName("ConstelacionI")))
        {
            UWorld* World = GetWorld();
            if (World && !World->GetTimerManager().IsTimerActive(TimerIntervaloConstelacion))
            {
                float Intervalo = Skills->ObtenerIntervaloSkill(FName("ConstelacionI"));
                if (Intervalo <= 0.0f) Intervalo = 1.0f;

                World->GetTimerManager().SetTimer(
                    TimerIntervaloConstelacion,
                    this,
                    &UUGI_Espacio::EjecutarConstelacionI,
                    Intervalo,
                    true
                );
                UE_LOG(LogTemp, Log, TEXT("[ConstelacionI] Ciclo pasivo iniciado cada %.1f segundos."), Intervalo);
            }
        }
    }
}

void UUGI_Espacio::EjecutarConstelacionI()
{
    USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>();
    if (!Skills) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // 1. Contar estrellas activas en este momento
    int32 CantidadEstrellas = 0;
    for (TActorIterator<AEstrellaActor> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            CantidadEstrellas++;
        }
    }

    // 2. Regla: por cada 5 estrellas
    const int32 GruposDeCinco = CantidadEstrellas / 5;
    if (GruposDeCinco > 0)
    {
        int32 PolvoBase = Skills->ObtenerCantidadSkill(FName("ConstelacionI"));
        if (PolvoBase <= 0) PolvoBase = 1;

        const int32 PolvoASumar = GruposDeCinco * PolvoBase;

        // Suma directa, sin multiplicadores de clic
        AddPolvo(PolvoASumar);

        UE_LOG(LogTemp, Verbose, TEXT("[ConstelacionI] Estrellas: %d | Grupos de 5: %d | Polvo sumado: %d"),
            CantidadEstrellas, GruposDeCinco, PolvoASumar);
    }
}

// =======================================================================
// AUTOGUARDADO
// =======================================================================
void UUGI_Espacio::AutoguardadoTick()
{
    GuardarPartida();
}

// =======================================================================
// GUARDAR PARTIDA
// =======================================================================
void UUGI_Espacio::GuardarPartida()
{
    UWorld* World = GetWorld();
    if (!World) return;

    USaveGameUniverso* SaveData = Cast<USaveGameUniverso>(UGameplayStatics::CreateSaveGameObject(USaveGameUniverso::StaticClass()));
    if (!SaveData) return;

    // 1. Datos globales
    SaveData->PolvoTotalGuardado = PolvoTotal;
    SaveData->NivelCamaraActualGuardado = NivelCamaraActual;
    SaveData->ContadorEstrellasGuardado = ContadorEstrellas;
    SaveData->EstrellasEvolucionadasGuardado = EstrellasEvolucionadas;
    SaveData->RadioExtraCreacionGuardado = RadioExtraCreacion;
    SaveData->SiguienteIDEstrellaGuardado = SiguienteIDEstrella;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 SiguienteIDEstrellaGuardado = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> SkillsCompradasGuardadas;


       // Skills compradas y sus niveles
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        SaveData->NivelesSkillsGuardados = Skills->ObtenerNivelesSkills();
    }

    // 2. Guardar la cámara
    if (ASpaceCameraPawn* Camara = Cast<ASpaceCameraPawn>(UGameplayStatics::GetPlayerPawn(World, 0)))
    {
        SaveData->PosicionCamara = Camara->GetActorLocation();
        SaveData->RotacionCamara = Camara->GetActorRotation();
        SaveData->TargetDistanceGuardada = Camara->TargetDistance;

        if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
        {
            SaveData->ControlRotationCamara = PC->GetControlRotation();
        }

        if (Camara->SpringArm)
        {
            SaveData->DistanciaCamara = Camara->SpringArm->TargetArmLength;
        }
        else
        {
            SaveData->DistanciaCamara = Camara->TargetDistance;
        }

        if (IsValid(Camara->TargetActor))
        {
            SaveData->bTieneTargetGuardado = true;
            SaveData->TargetPosicionGuardada = Camara->TargetActor->GetActorLocation();
        }
        else
        {
            SaveData->bTieneTargetGuardado = false;
        }
    }

    // 3. Guardar estrellas
    TArray<AActor*> EstrellasEncontradas;
    UGameplayStatics::GetAllActorsOfClass(World, AEstrellaActor::StaticClass(), EstrellasEncontradas);

    for (AActor* Actor : EstrellasEncontradas)
    {
        if (AEstrellaActor* Estrella = Cast<AEstrellaActor>(Actor))
        {
            FEstrellaGuardada DatosEstrella;
            DatosEstrella.ClaseEstrella = Estrella->GetClass();
            DatosEstrella.Transform = Estrella->GetActorTransform();
            DatosEstrella.EtapaEvolucion = Estrella->EtapaEvolucion;
            DatosEstrella.TipoEstrella = Estrella->TipoEstrella;
            DatosEstrella.IDEstrella = Estrella->IDEstrella;
            DatosEstrella.bAnimacionInicialTerminada = Estrella->bAnimacionInicialTerminada;
            DatosEstrella.bBoton0Usado = Estrella->bBoton0Usado;
            DatosEstrella.bBoton1Usado = Estrella->bBoton1Usado;
            DatosEstrella.bBoton2Usado = Estrella->bBoton2Usado;
            DatosEstrella.bBoton3Usado = Estrella->bBoton3Usado;
            DatosEstrella.bBoton4Usado = Estrella->bBoton4Usado;
            DatosEstrella.bBoton5Usado = Estrella->bBoton5Usado;
            DatosEstrella.bBoton6Usado = Estrella->bBoton6Usado;
            DatosEstrella.bBoton7Usado = Estrella->bBoton7Usado;
            DatosEstrella.bBoton8Usado = Estrella->bBoton8Usado;
            DatosEstrella.bBoton9Usado = Estrella->bBoton9Usado;
            DatosEstrella.bBoton10Usado = Estrella->bBoton10Usado;

            DatosEstrella.bEcoCosmicoActivo = Estrella->bEcoCosmicoActivo;
            DatosEstrella.MultiplicadorEco = Estrella->MultiplicadorEco;


            SaveData->EstrellasEnElMapa.Add(DatosEstrella);
        }
    }

    // 4. Guardar polvos
    TArray<AActor*> PolvosEncontrados;
    UGameplayStatics::GetAllActorsWithTag(World, FName("PolvoEstelar"), PolvosEncontrados);
    for (AActor* ActorPolvo : PolvosEncontrados)
    {
        if (IsValid(ActorPolvo))
        {
            FPolvoGuardado DatosPolvo;
            DatosPolvo.ClasePolvo = ActorPolvo->GetClass();
            DatosPolvo.Transform = ActorPolvo->GetActorTransform();
            SaveData->PolvosEnElMapa.Add(DatosPolvo);
        }
    }

    UGameplayStatics::SaveGameToSlot(SaveData, NombreSlotGuardado, 0);
    UE_LOG(LogTemp, Log, TEXT("Partida guardada con éxito."));
}

// =======================================================================
// CARGAR PARTIDA
// =======================================================================
void UUGI_Espacio::CargarPartida()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (!UGameplayStatics::DoesSaveGameExist(NombreSlotGuardado, 0)) return;

    USaveGameUniverso* SaveData = Cast<USaveGameUniverso>(UGameplayStatics::LoadGameFromSlot(NombreSlotGuardado, 0));
    if (!SaveData) return;

    // 1. Restaurar variables globales
    PolvoTotal = SaveData->PolvoTotalGuardado;
    NivelCamaraActual = SaveData->NivelCamaraActualGuardado;
    ContadorEstrellas = SaveData->ContadorEstrellasGuardado;
    EstrellasEvolucionadas = SaveData->EstrellasEvolucionadasGuardado;
    RadioExtraCreacion = SaveData->RadioExtraCreacionGuardado;
    SiguienteIDEstrella = SaveData->SiguienteIDEstrellaGuardado;
    

    // Restaurar las skills compradas y sus niveles
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        Skills->CargarNivelesSkills(SaveData->NivelesSkillsGuardados); // 👈 Nueva función
    }


    InicializarNebulosaSuperGalacticaSiCorresponde();

    // Reiniciar los timers de las habilidades pasivas que ya estaban compradas
    InicializarLluviaDePolvoSiCorresponde();
    InicializarConstelacionISiCorresponde();
    InicializarEstrellaDormidaSiCorresponde();

    OnPolvoCambiado.Broadcast(PolvoTotal);
    OnNivelCamaraCambiado.Broadcast(NivelCamaraActual);

    // 2. Destruir estrellas y polvos actuales
    TArray<AActor*> EstrellasActuales;
    UGameplayStatics::GetAllActorsOfClass(World, AEstrellaActor::StaticClass(), EstrellasActuales);
    for (AActor* Actor : EstrellasActuales) Actor->Destroy();

    TArray<AActor*> PolvosActuales;
    UGameplayStatics::GetAllActorsWithTag(World, FName("PolvoEstelar"), PolvosActuales);
    for (AActor* P : PolvosActuales) P->Destroy();

    // 3. Spawnear Estrellas y buscar la que era Target
    AEstrellaActor* EstrellaTargetCargada = nullptr;
    for (const FEstrellaGuardada& DatosEstrella : SaveData->EstrellasEnElMapa)
    {
        if (DatosEstrella.ClaseEstrella)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            SpawnParams.bDeferConstruction = true;

            AEstrellaActor* NuevaEstrella = World->SpawnActor<AEstrellaActor>(DatosEstrella.ClaseEstrella, DatosEstrella.Transform, SpawnParams);
            if (NuevaEstrella)
            {
                NuevaEstrella->EtapaEvolucion = DatosEstrella.EtapaEvolucion;
                NuevaEstrella->bAnimacionInicialTerminada = DatosEstrella.bAnimacionInicialTerminada;

                // Restaurar Tipo e ID
                NuevaEstrella->TipoEstrella = DatosEstrella.TipoEstrella;
                NuevaEstrella->IDEstrella = DatosEstrella.IDEstrella;

                NuevaEstrella->bBoton0Usado = DatosEstrella.bBoton0Usado;
                NuevaEstrella->bBoton1Usado = DatosEstrella.bBoton1Usado;
                NuevaEstrella->bBoton2Usado = DatosEstrella.bBoton2Usado;
                NuevaEstrella->bBoton3Usado = DatosEstrella.bBoton3Usado;
                NuevaEstrella->bBoton4Usado = DatosEstrella.bBoton4Usado;
                NuevaEstrella->bBoton5Usado = DatosEstrella.bBoton5Usado;
                NuevaEstrella->bBoton6Usado = DatosEstrella.bBoton6Usado;
                NuevaEstrella->bBoton7Usado = DatosEstrella.bBoton7Usado;
                NuevaEstrella->bBoton8Usado = DatosEstrella.bBoton8Usado;
                NuevaEstrella->bBoton9Usado = DatosEstrella.bBoton9Usado;
                NuevaEstrella->bBoton10Usado = DatosEstrella.bBoton10Usado;

                NuevaEstrella->bEcoCosmicoActivo = DatosEstrella.bEcoCosmicoActivo;
                NuevaEstrella->MultiplicadorEco = DatosEstrella.MultiplicadorEco;

                // Le decimos a la estrella que viene de un SaveGame
                NuevaEstrella->bCargadaDesdeSave = true;

                NuevaEstrella->FinishSpawning(DatosEstrella.Transform);

                if (SaveData->bTieneTargetGuardado)
                {
                    if (FVector::Dist(NuevaEstrella->GetActorLocation(), SaveData->TargetPosicionGuardada) < 100.0f)
                    {
                        EstrellaTargetCargada = NuevaEstrella;
                    }
                }
            }
        }
    }

    // 4. Spawnear Polvos
    for (const FPolvoGuardado& DatosPolvo : SaveData->PolvosEnElMapa)
    {
        if (!DatosPolvo.ClasePolvo) continue;

        // 4.1. Estrella más cercana al polvo
        AEstrellaActor* EstrellaCercana = nullptr;
        float MejorDistSq = TNumericLimits<float>::Max();
        for (TActorIterator<AEstrellaActor> It(World); It; ++It)
        {
            if (!IsValid(*It)) continue;
            const float DistSq = FVector::DistSquared(
                It->GetActorLocation(),
                DatosPolvo.Transform.GetLocation());
            if (DistSq < MejorDistSq)
            {
                MejorDistSq = DistSq;
                EstrellaCercana = *It;
            }
        }

        // 4.2. Spawn diferido para asignar Owner ANTES del BeginPlay
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParams.bDeferConstruction = true;

        AActor* NuevoPolvo = World->SpawnActor<AActor>(
            DatosPolvo.ClasePolvo, DatosPolvo.Transform, SpawnParams);

        if (!NuevoPolvo) continue;

        if (EstrellaCercana)
        {
            NuevoPolvo->SetOwner(EstrellaCercana);
        }

        NuevoPolvo->FinishSpawning(DatosPolvo.Transform);

        // 4.3. Registrar el polvo en el generador de esa estrella
        if (EstrellaCercana)
        {
            if (UBPC_GeneradorPolvo* Generador = EstrellaCercana->FindComponentByClass<UBPC_GeneradorPolvo>())
            {
                Generador->RegistrarPolvoExistente(NuevoPolvo);
            }
        }

        // 4.4. Efecto visual
        if (UBPC_EfectoPolvo* Efecto = NuevoPolvo->FindComponentByClass<UBPC_EfectoPolvo>())
        {
            Efecto->IniciarEfecto(DatosPolvo.Transform.GetScale3D());
        }

        
    }

    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        const int32 NivelCinturon = Skills->ObtenerNivelSkill(FName("CinturonAsteroides"));
        for (int32 i = 0; i < NivelCinturon; ++i)
        {
            ActivarCinturonEnEstrellaAleatoria();
        }
    }


    // 5. Restaurar la cámara
    if (ASpaceCameraPawn* Camara = Cast<ASpaceCameraPawn>(UGameplayStatics::GetPlayerPawn(World, 0)))
    {
        Camara->bGameLoaded = true;

        Camara->SetActorLocation(SaveData->PosicionCamara);

        if (EstrellaTargetCargada)
        {
            Camara->TargetActor = EstrellaTargetCargada;
        }
        else
        {
            Camara->TargetActor = nullptr;
        }

        if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
        {
            PC->SetControlRotation(SaveData->ControlRotationCamara);
        }

        float DistanciaCargada = SaveData->TargetDistanceGuardada;
        if (DistanciaCargada <= 0.0f) DistanciaCargada = SaveData->DistanciaCamara;
        if (DistanciaCargada <= 0.0f) DistanciaCargada = 2000.0f;

        Camara->TargetDistance = DistanciaCargada;
        if (Camara->SpringArm)
        {
            Camara->SpringArm->TargetArmLength = DistanciaCargada;
        }

        // Avisar al HUD para que actualice los botones
        if (Camara->HUDRef)
        {
            if (UFunction* FuncActualizar = Camara->HUDRef->FindFunction(FName("ActualizarEstadoBotones")))
            {
                Camara->HUDRef->ProcessEvent(FuncActualizar, nullptr);
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Partida cargada. Target encontrado: %s"), EstrellaTargetCargada ? TEXT("SI") : TEXT("NO"));
}

// =======================================================================
// EXISTE PARTIDA GUARDADA
// =======================================================================
bool UUGI_Espacio::ExistePartidaGuardada() const
{
    return UGameplayStatics::DoesSaveGameExist(NombreSlotGuardado, 0);
}

// =======================================================================
// NIVEL DE CÁMARA
// =======================================================================
bool UUGI_Espacio::AvanzarNivelCamara()
{
    if (NivelCamaraActual < NivelCamaraMaximo)
    {
        NivelCamaraActual++;
        IncrementarEstrellas(1);
        OnNivelCamaraCambiado.Broadcast(NivelCamaraActual);
        return true;
    }
    return false;
}

void UUGI_Espacio::IncrementarEstrellasEvolucionadas(int32 Cantidad) { EstrellasEvolucionadas += Cantidad; }

float UUGI_Espacio::GetMinDistanceNivelActual() const
{
    const int32 Index = FMath::Clamp(NivelCamaraActual, 0, DistanciasMinimasPorNivel.Num() - 1);
    return DistanciasMinimasPorNivel.IsValidIndex(Index) ? DistanciasMinimasPorNivel[Index] : 1000.0f;
}

float UUGI_Espacio::GetMaxDistanceNivelActual() const
{
    const int32 Index = FMath::Clamp(NivelCamaraActual, 0, DistanciasMaximasPorNivel.Num() - 1);
    return DistanciasMaximasPorNivel.IsValidIndex(Index) ? DistanciasMaximasPorNivel[Index] : 5000.0f;
}

float UUGI_Espacio::GetMinRadiusNivelActual() const
{
    const int32 Index = FMath::Clamp(NivelCamaraActual, 0, RadiosMinimosPorNivel.Num() - 1);
    return RadiosMinimosPorNivel.IsValidIndex(Index) ? RadiosMinimosPorNivel[Index] : 2000.0f;
}

float UUGI_Espacio::GetMaxRadiusNivelActual() const
{
    const int32 Index = FMath::Clamp(NivelCamaraActual, 0, RadiosMaximosPorNivel.Num() - 1);
    const float RadioBase = RadiosMaximosPorNivel.IsValidIndex(Index) ? RadiosMaximosPorNivel[Index] : 10000.0f;
    return RadioBase + RadioExtraCreacion;
}

void UUGI_Espacio::AgregarRadioExtra(float CantidadExtra) { RadioExtraCreacion += CantidadExtra; }
void UUGI_Espacio::IncrementarEstrellas(int32 Cantidad) { ContadorEstrellas += Cantidad; }

bool UUGI_Espacio::DebeMostrarContenidoEstrellas() const
{
    return (ContadorEstrellas >= 5) || (NivelCamaraActual >= 5);
}

// =======================================================================
// IDs DE ESTRELLAS Y NÚCLEO CALIENTE
// =======================================================================
int32 UUGI_Espacio::ObtenerSiguienteIDEstrella()
{
    const int32 IDActual = SiguienteIDEstrella;
    SiguienteIDEstrella++;
    return IDActual;
}

bool UUGI_Espacio::EsEstrellaAntigua(AActor* Estrella)
{
    if (!Estrella) return false;

    AEstrellaActor* EstrellaTarget = Cast<AEstrellaActor>(Estrella);
    if (!EstrellaTarget || EstrellaTarget->IDEstrella <= 0) return false;

    // 1. Verificar que el jugador tenga la skill Núcleo Caliente
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        if (!Skills->EsNucleoCalienteActiva()) return false;
    }
    else
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World) return false;

    // 2. Recopilar todas las estrellas vivas
    TArray<AEstrellaActor*> EstrellasVivas;
    for (TActorIterator<AEstrellaActor> It(World); It; ++It)
    {
        if (IsValid(*It)) EstrellasVivas.Add(*It);
    }

    // 3. Ordenar de más vieja a más nueva (ID de menor a mayor)
    EstrellasVivas.Sort([](const AEstrellaActor& A, const AEstrellaActor& B)
        {
            return A.IDEstrella < B.IDEstrella;
        });

    // 4. ¿Está entre las 3 más viejas?
    const int32 Limite = FMath::Min(3, EstrellasVivas.Num());
    for (int32 i = 0; i < Limite; ++i)
    {
        if (EstrellasVivas[i] == EstrellaTarget) return true;
    }

    return false;
}

// =======================================================================
// RECURSOS
// =======================================================================
void UUGI_Espacio::AddPolvo(int32 Cantidad)
{
    PolvoTotal += Cantidad;
    OnPolvoCambiado.Broadcast(PolvoTotal);
}

void UUGI_Espacio::SetPolvoTotal(int32 NuevoPolvo)
{
    PolvoTotal = NuevoPolvo;
    OnPolvoCambiado.Broadcast(PolvoTotal);
}

void UUGI_Espacio::NotificarCambioPolvo() { OnPolvoCambiado.Broadcast(PolvoTotal); }

void UUGI_Espacio::AddPolvoConSkills(int32 PolvoBase)
{
    int32 PolvoFinal = PolvoBase;

    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        PolvoFinal = Skills->CalcularPolvoGanado(PolvoBase);
    }

    // Multiplicador de habilidades de rebirth (Universo Joven), acumulando la fracción
    const float PolvoConBono = (PolvoFinal * MultiplicadorProduccion) + RestoPolvoFraccion;
    PolvoFinal = FMath::FloorToInt(PolvoConBono);
    RestoPolvoFraccion = PolvoConBono - PolvoFinal;

    AddPolvo(PolvoFinal);
}

void UUGI_Espacio::AddPolvoPasivo(int32 PolvoBase)
{
    // Solo aplica el multiplicador de Universo Joven (MultiplicadorProduccion), NO el de clic.
    const float PolvoConBono = (PolvoBase * MultiplicadorProduccion) + RestoPolvoFraccion;
    int32 PolvoFinal = FMath::FloorToInt(PolvoConBono);
    RestoPolvoFraccion = PolvoConBono - PolvoFinal;

    AddPolvo(PolvoFinal);
}

// =======================================================================
// RESET DE PARTIDA
// =======================================================================
void UUGI_Espacio::ResetearPartida(bool bBorrarGuardado, bool bSpawnearEstrellaInicial)
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. BORRAR EL ARCHIVO DE GUARDADO
    if (bBorrarGuardado && UGameplayStatics::DoesSaveGameExist(NombreSlotGuardado, 0))
    {
        UGameplayStatics::DeleteGameInSlot(NombreSlotGuardado, 0);
        UE_LOG(LogTemp, Log, TEXT("[Reset] Archivo de guardado eliminado."));
    }

    // 2. RESETEAR VARIABLES GLOBALES
    PolvoTotal = 0;
    NivelCamaraActual = 0;
    ContadorEstrellas = 1;
    EstrellasEvolucionadas = 0;
    RadioExtraCreacion = 0.0f;
    SiguienteIDEstrella = 1;

    // Reiniciar las skills y detener sus timers
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        Skills->ResetearSkills();
    }
    World->GetTimerManager().ClearTimer(TimerIntervaloLluvia);
    World->GetTimerManager().ClearTimer(TimerIntervaloConstelacion);
    World->GetTimerManager().ClearTimer(TimerIntervaloEstrellaDormida); // 👈 Añadido
    World->GetTimerManager().ClearTimer(TimerNebulosaSuperGalactica);

    // 3. DESTRUIR TODAS LAS ESTRELLAS
    TArray<AActor*> EstrellasActuales;
    UGameplayStatics::GetAllActorsOfClass(World, AEstrellaActor::StaticClass(), EstrellasActuales);
    for (AActor* Actor : EstrellasActuales)
    {
        if (IsValid(Actor)) Actor->Destroy();
    }

    // 4. DESTRUIR TODO EL POLVO
    TArray<AActor*> PolvosActuales;
    UGameplayStatics::GetAllActorsWithTag(World, FName("PolvoEstelar"), PolvosActuales);
    for (AActor* P : PolvosActuales)
    {
        if (IsValid(P)) P->Destroy();
    }

    // 5. RESETEAR LA CÁMARA
    if (ASpaceCameraPawn* Camara = Cast<ASpaceCameraPawn>(UGameplayStatics::GetPlayerPawn(World, 0)))
    {
        Camara->TargetActor = nullptr;
        Camara->bGameLoaded = false;
        Camara->bMenuInferiorAbierto = false;
        Camara->bEstaArrastrandoPolvo = false;
        Camara->StarMenuDistances.Empty();

        const float DistInicial = Camara->MinDistance;
        Camara->TargetDistance = DistInicial;
        if (Camara->SpringArm)
        {
            Camara->SpringArm->TargetArmLength = DistInicial;
        }

        Camara->SetActorLocation(FVector::ZeroVector);

        if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
        {
            PC->SetControlRotation(FRotator::ZeroRotator);
        }

        // Avisar al HUD
        if (Camara->HUDRef)
        {
            if (UFunction* FuncCerrar = Camara->HUDRef->FindFunction(FName("CerrarMenuInferior")))
            {
                Camara->HUDRef->ProcessEvent(FuncCerrar, nullptr);
            }
            if (UFunction* FuncAct = Camara->HUDRef->FindFunction(FName("ActualizarEstadoBotones")))
            {
                Camara->HUDRef->ProcessEvent(FuncAct, nullptr);
            }
        }
    }

    // 6. RESPAWNEAR LA ESTRELLA INICIAL (opcional)
    if (bSpawnearEstrellaInicial && ClaseEstrellaInicial)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AEstrellaActor* NuevaInicial = World->SpawnActor<AEstrellaActor>(
            ClaseEstrellaInicial,
            UbicacionEstrellaInicial,
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (NuevaInicial)
        {
            NuevaInicial->EtapaEvolucion = 0;
            NuevaInicial->bAnimacionInicialTerminada = false;
            UE_LOG(LogTemp, Log, TEXT("[Reset] Estrella inicial respawneada."));
        }
    }

    // 7. NOTIFICAR AL HUD / UI
    OnPolvoCambiado.Broadcast(PolvoTotal);
    OnNivelCamaraCambiado.Broadcast(NivelCamaraActual);

    // 8. REINICIAR EL TIMER DE AUTOGUARDADO
    World->GetTimerManager().ClearTimer(TimerAutoguardado);
    World->GetTimerManager().SetTimer(
        TimerAutoguardado,
        this,
        &UUGI_Espacio::AutoguardadoTick,
        TiempoAutoguardado,
        true
    );

    UE_LOG(LogTemp, Warning, TEXT("[Reset] Partida reseteada por completo."));
}


void UUGI_Espacio::EjecutarResetGalactico()
{
    // 1. Calcular la Esencia basada en el polvo actual
    int32 EsenciaGanada = FMath::RoundToInt(PolvoTotal * (PorcentajeEsenciaGalactica / 100.0f));

    // 2. Avisar al Blueprint para que sume la Esencia
    OnResetGalactico(EsenciaGanada);

    // 3. Reiniciar la partida (Polvo a 0, destruir estrellas, resetear skills)
    // Usamos 'false' en bBorrarGuardado para que no borre el archivo del disco de golpe.
    ResetearPartida(false, true);

    UE_LOG(LogTemp, Warning, TEXT("[Reset Galactico] ¡Prestigio realizado! Esencia ganada: %d"), EsenciaGanada);
}



void UUGI_Espacio::InicializarEstrellaDormidaSiCorresponde()
{
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        if (Skills->EsEstrellaDormidaActiva())
        {
            UWorld* World = GetWorld();
            if (World && !World->GetTimerManager().IsTimerActive(TimerIntervaloEstrellaDormida))
            {
                // Leemos el IntervaloTiempo de la DataTable (cada cuántos segundos intenta hacer la tirada)
                float Intervalo = Skills->ObtenerIntervaloSkill(FName("EstrellaDormida"));
                if (Intervalo <= 0.0f) Intervalo = 30.0f;

                World->GetTimerManager().SetTimer(
                    TimerIntervaloEstrellaDormida,
                    this,
                    &UUGI_Espacio::EjecutarEstrellaDormida,
                    Intervalo,
                    true // Repetir constantemente
                );
                UE_LOG(LogTemp, Log, TEXT("[EstrellaDormida] Ciclo iniciado cada %.1f segundos."), Intervalo);
            }
        }
    }
}

void UUGI_Espacio::EjecutarEstrellaDormida()
{
    USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>();
    if (!Skills || !Skills->EsEstrellaDormidaActiva()) return;

    // Tirada de probabilidad
    int32 Probabilidad = Skills->ObtenerProbabilidadEstrellaDormida();
    if (FMath::RandRange(1, 100) > Probabilidad) return; // Falló la tirada, no aparece nada

    UWorld* World = GetWorld();
    if (!World || !ClaseEstrellaInicial) return;

    // ¿Dónde va a aparecer? Según el nivel de cámara actual
    float MinRadius = GetMinRadiusNivelActual();
    float MaxRadius = GetMaxRadiusNivelActual();

    FVector UbicacionAleatoria = FVector::ZeroVector;
    bool bEncontrada = false;

    // Usamos la misma lógica que el sistema orbital para que no aparezca encima de otra estrella
    float RadioOrbita = UBPC_OrbitaComponent::GetRadioOrbitaDeClase(ClaseEstrellaInicial);
    float MinSeparation = (RadioOrbita > 0.0f) ? RadioOrbita : 500.0f;

    bEncontrada = UBPC_OrbitaComponent::EncontrarPosicionOrbitalValida(
        this, MinRadius, MaxRadius, MinSeparation, 30, UbicacionAleatoria
    );

    // Si no encontró sitio libre (mapa muy lleno), la ponemos en un lugar aleatorio igual
    if (!bEncontrada)
    {
        float Angulo = FMath::FRandRange(0.0f, 360.0f);
        float Distancia = FMath::FRandRange(MinRadius, MaxRadius);
        float Radianes = FMath::DegreesToRadians(Angulo);

        UbicacionAleatoria.X = FMath::Cos(Radianes) * Distancia;
        UbicacionAleatoria.Y = FMath::Sin(Radianes) * Distancia;
        UbicacionAleatoria.Z = FMath::FRandRange(-1000.0f, 1000.0f);
    }

    // Spawneamos la estrella
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AEstrellaActor* NuevaEstrella = World->SpawnActor<AEstrellaActor>(
        ClaseEstrellaInicial,
        UbicacionAleatoria,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (NuevaEstrella)
    {
        NuevaEstrella->EtapaEvolucion = 0;
        NuevaEstrella->bAnimacionInicialTerminada = false;

        // Sumamos al contador global de estrellas
        IncrementarEstrellas(1);

        UE_LOG(LogTemp, Warning, TEXT("[EstrellaDormida] ¡Una estrella ha aparecido espontáneamente!"));
    }
}

void UUGI_Espacio::ActivarCinturonEnEstrellaAleatoria()
{
    UWorld* World = GetWorld();
    if (!World) return;

    USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>();
    if (!Skills || !Skills->EsCinturonAsteroidesActivo()) return;

    // Recolectar cinturones inactivos
    TArray<UBPC_CinturonAsteroides*> Candidatos;
    for (TActorIterator<AEstrellaActor> It(World); It; ++It)
    {
        if (!IsValid(*It)) continue;
        if (UBPC_CinturonAsteroides* Cint = It->FindComponentByClass<UBPC_CinturonAsteroides>())
        {
            if (!Cint->EstaActivo())
            {
                Candidatos.Add(Cint);
            }
        }
    }

    if (Candidatos.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[Cinturon] No hay estrellas libres para activar."));
        return;
    }

    const int32 Index = FMath::RandRange(0, Candidatos.Num() - 1);
    Candidatos[Index]->ActivarCinturon();
}

// ✅ DEJA SOLO ESTA (al final del archivo)
void UUGI_Espacio::OnSkillCompradaRecibida(FName IdSkill)
{
    if (IdSkill == FName("EstrellaDormida"))
    {
        InicializarEstrellaDormidaSiCorresponde();
    }
    else if (IdSkill == FName("CinturonAsteroides"))
    {
        ActivarCinturonEnEstrellaAleatoria();
    }

    else if (IdSkill == FName("NebulosaSuperGalactica"))
    {
        InicializarNebulosaSuperGalacticaSiCorresponde();
    }
}

// En UGI_Espacio.cpp
int32 UUGI_Espacio::ObtenerCantidadNebulosas() const
{
    int32 Cantidad = 0;
    UWorld* World = GetWorld();
    if (!World) return 0;

    for (TActorIterator<AEstrellaActor> It(World); It; ++It)
    {
        if (IsValid(*It) && It->bEsNebulosa)
        {
            Cantidad++;
        }
    }
    return Cantidad;
}

void UUGI_Espacio::InicializarNebulosaSuperGalacticaSiCorresponde()
{
    if (USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>())
    {
        if (Skills->EsNebulosaSuperGalacticaActiva())
        {
            UWorld* World = GetWorld();
            if (World && !World->GetTimerManager().IsTimerActive(TimerNebulosaSuperGalactica))
            {
                // Cada cuántos segundos se hace la tirada (Lo leemos de la DataTable)
                float Intervalo = Skills->ObtenerIntervaloSkill(FName("NebulosaSuperGalactica"));
                if (Intervalo <= 0.0f) Intervalo = 10.0f; // Por defecto 10 segundos

                World->GetTimerManager().SetTimer(
                    TimerNebulosaSuperGalactica,
                    this,
                    &UUGI_Espacio::EjecutarEvolucionNebulosa,
                    Intervalo,
                    true
                );
                UE_LOG(LogTemp, Log, TEXT("[NebulosaSuperGalactica] Ciclo de evolución automática iniciado cada %.1f segundos."), Intervalo);
            }
        }
    }
}

void UUGI_Espacio::EjecutarEvolucionNebulosa()
{
    USkillsSubsystem* Skills = GetSubsystem<USkillsSubsystem>();
    if (!Skills || !Skills->EsNebulosaSuperGalacticaActiva()) return;

    // 1. Contar nebulosas activas para calcular la probabilidad
    int32 CantNebulosas = ObtenerCantidadNebulosas();
    if (CantNebulosas <= 0) return;

    float ProbabilidadPorNebulosa = Skills->ObtenerProbabilidadEvolucionNebulosa();
    float ProbabilidadTotal = ProbabilidadPorNebulosa * CantNebulosas;

    if (ProbabilidadTotal <= 0.0f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // 2. Recolectar TODAS las estrellas que puedan evolucionar
    TArray<AEstrellaActor*> Candidatas;
    for (TActorIterator<AEstrellaActor> It(World); It; ++It)
    {
        AEstrellaActor* Estrella = *It;
        // Ignoramos nebulosas y estrellas que aún no terminan su animación de nacimiento
        if (IsValid(Estrella) && !Estrella->bEsNebulosa && Estrella->bAnimacionInicialTerminada)
        {
            Candidatas.Add(Estrella);
        }
    }

    if (Candidatas.Num() == 0) return;

    // 3. Hacer UNA SOLA TIRADA GLOBAL (de 0 a 100)
    float Tirada = FMath::FRandRange(0.0f, 100.0f);

    if (Tirada <= ProbabilidadTotal)
    {
        // 4. Si la tirada es exitosa, ordenamos por ID (la más antigua primero)
        Candidatas.Sort([](const AEstrellaActor& A, const AEstrellaActor& B)
            {
                return A.IDEstrella < B.IDEstrella;
            });

        // 5. Elegimos SOLO la primera de la lista (la más antigua)
        AEstrellaActor* EstrellaElegida = Candidatas[0];

        EstrellaElegida->EvolucionarAutomaticamente();

        UE_LOG(LogTemp, Warning, TEXT("[NebulosaSuperGalactica] ¡Tirada exitosa (%.1f)! La estrella ID %d ha evolucionado sola."),
            Tirada, EstrellaElegida->IDEstrella);
    }
} 

