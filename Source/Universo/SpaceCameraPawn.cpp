// Copyright (c) 2026 Universo. All Rights Reserved.

#include "SpaceCameraPawn.h"
#include "EvolutionComponent.h"
#include "EstrellaActor.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BillboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "UGI_Espacio.h"
#include "Blueprint/UserWidget.h"

#include "BPC_GeneradorPolvo.h"
#include "DrawDebugHelpers.h"
#include "SkillsSubsystem.h"


// =======================================================================
// CONSTRUCTOR
// =======================================================================

ASpaceCameraPawn::ASpaceCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = TargetDistance;
    SpringArm->bDoCollisionTest = true;
    SpringArm->ProbeSize = 30.0f;
    SpringArm->bUsePawnControlRotation = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;



    // --- Sprite 2D del cursor (Billboard) ---
    SpriteCursorComp = CreateDefaultSubobject<UBillboardComponent>(TEXT("SpriteCursorComp"));
    SpriteCursorComp->SetupAttachment(RootComponent);
    SpriteCursorComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpriteCursorComp->SetVisibility(false);
    SpriteCursorComp->SetHiddenInGame(true);
    SpriteCursorComp->bIsScreenSizeScaled = false;
    SpriteCursorComp->SetAbsolute(true, true, true);
}

// =======================================================================
// CICLO DE VIDA
// =======================================================================

void ASpaceCameraPawn::BeginPlay()
{
    Super::BeginPlay(); // 👈 El Blueprint se ejecuta AQUÍ y llama a CargarPartida()

    GI = Cast<UUGI_Espacio>(UGameplayStatics::GetGameInstance(this));
    if (GI)
    {
        GI->OnNivelCamaraCambiado.AddDynamic(this, &ASpaceCameraPawn::HandleNivelCamaraCambiado);

        // 👇 NUEVO: Solo aplicamos la distancia del nivel si NO cargamos partida
        if (!bGameLoaded)
        {
            AplicarNivelCamara();
        }
    }

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;
        PC->HitResultTraceDistance = 3000000.0f;
    }

    if (SpriteCursorComp)
    {
        if (SpriteCursorTexture)
        {
            SpriteCursorComp->SetSprite(SpriteCursorTexture);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SpaceCamera] SpriteCursorTexture no está asignada en BP_SpaceCamera."));
        }

        SpriteCursorComp->SetWorldScale3D(SpriteCursorScale);
        SpriteCursorComp->SetAbsolute(true, true, true);
    }
}

// =======================================================================
// FUNCIÓN AUXILIAR (Para leer el valor del polvo desde Blueprint o C++)
// =======================================================================
int32 ASpaceCameraPawn::ObtenerValorPolvo(AActor* HitActor)
{
    if (!IsValid(HitActor)) return 1;

    // 1. Generador del Owner (LA FUENTE PRINCIPAL)
    if (AActor* OwnerEstrella = HitActor->GetOwner())
    {
        if (UBPC_GeneradorPolvo* Generador = OwnerEstrella->FindComponentByClass<UBPC_GeneradorPolvo>())
        {
            return Generador->ObtenerValorDePolvo(HitActor);
        }
    }

    // 2. Fallback: propiedad en el actor
    if (FIntProperty* Prop = CastField<FIntProperty>(
        HitActor->GetClass()->FindPropertyByName(TEXT("ValorPolvo"))))
    {
        return Prop->GetPropertyValue_InContainer(HitActor);
    }

    return 1;
}

void ASpaceCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!SpringArm) return;

    if (IsValid(TargetActor) && bEncuadrarAlSeleccionar)
    {
        const float MinDist = GetMinDistanciaPermitida();

        if (TargetDistance < MinDist)
        {
            TargetDistance = FMath::FInterpTo(TargetDistance, MinDist, DeltaSeconds, InterpSpeed);
        }
    }

    const float CurrentArm = SpringArm->TargetArmLength;
    SpringArm->TargetArmLength = FMath::FInterpTo(CurrentArm, TargetDistance, DeltaSeconds, InterpSpeed);

    if (IsValid(TargetActor))
    {
        const FVector TargetLoc = TargetActor->GetActorLocation();
        const FVector CurrentLoc = GetActorLocation();
        const FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaSeconds, InterpSpeed);
        SetActorLocation(NewLoc);
    }

    USkillsSubsystem* Skills = GI ? GI->GetSubsystem<USkillsSubsystem>() : nullptr;

    // ===================================================================
   // 👇 DIBUJADO DEL RANGO DE RECOLECCIÓN (DEBUG) 👇
   // ===================================================================
    if (Skills && Skills->bAspiradoPolvoActivo)
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            FVector WorldLocation, WorldDirection;
            if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
            {
                const FVector PosicionCursor = WorldLocation + (WorldDirection * TargetDistance);

                DrawDebugSphere(
                    GetWorld(),
                    PosicionCursor,
                    Skills->ObtenerRadioMaximoAspirado(),
                    32,
                    FColor::Green,
                    false,
                    -1.0f,
                    0,
                    1.5f
                );

                if (bEstaArrastrandoPolvo)
                {
                    DrawDebugSphere(
                        GetWorld(),
                        PosicionCursor,
                        RadioAspiradoActual,
                        24,
                        FColor::Yellow,
                        false,
                        -1.0f,
                        0,
                        2.5f
                    );
                }
            }
        }
    }
    // ===================================================================
    // 👆 FIN DEL DIBUJADO DEL RANGO 👆
    // ===================================================================



     // ===================================================================
    // RECOLECCIÓN DE POLVO (arrastrando clic)
    // ===================================================================
    if (bEstaArrastrandoPolvo && Skills && Skills->bAspiradoPolvoActivo)
    {
        if (PolvosDestruidosEnArrastre >= Skills->MaxPolvoPorAccion)
        {
            SetSpriteCursorVisible(false);
        }
        else
        {
            SetSpriteCursorVisible(true);
            UpdateSpriteCursorPosition();

            RadioAspiradoActual = FMath::Min(
                RadioAspiradoActual + (Skills->VelocidadExpansion * DeltaSeconds),
                Skills->ObtenerRadioMaximoAspirado()

            );

            if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
            {
                FVector WorldLocation, WorldDirection;
                if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
                {
                    const FVector Start = WorldLocation;
                    const FVector End = WorldLocation + (WorldDirection * 100000.0f);

                    TArray<FHitResult> Hits;
                    FCollisionQueryParams QueryParams;
                    QueryParams.bTraceComplex = false;
                    QueryParams.AddIgnoredActor(this);

                    FCollisionShape SweepShape = FCollisionShape::MakeSphere(RadioAspiradoActual);

                    FCollisionObjectQueryParams ObjectParams;
                    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
                    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
                    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
                    ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
                    ObjectParams.AddObjectTypesToQuery(ECC_Vehicle);
                    ObjectParams.AddObjectTypesToQuery(ECC_Destructible);

                    bool bHit = GetWorld()->SweepMultiByObjectType(
                        Hits,
                        Start,
                        End,
                        FQuat::Identity,
                        ObjectParams,
                        SweepShape,
                        QueryParams
                    );

                    if (bHit)
                    {
                        int32 PolvoBaseRecolectado = 0;

                        for (const FHitResult& Hit : Hits)
                        {
                            if (PolvosDestruidosEnArrastre >= Skills->MaxPolvoPorAccion)
                            {
                                break;
                            }

                            AActor* HitActor = Hit.GetActor();
                            if (!IsValid(HitActor)) continue;

                            const bool bEsPolvo = HitActor->GetName().Contains(TEXT("Polvo")) ||
                                HitActor->ActorHasTag(TEXT("PolvoEstelar"));

                            if (!bEsPolvo) continue;

                            int32 ValorPolvoObtenido = ObtenerValorPolvo(HitActor);

                            HitActor->Destroy();
                            PolvosDestruidosEnArrastre++;

                            PolvoBaseRecolectado += ValorPolvoObtenido;
                        }

                        if (GI && PolvoBaseRecolectado > 0)
                        {
                            GI->AddPolvoConSkills(PolvoBaseRecolectado);
                        }
                    }
                }
            }
        }
    }
    else
    {
        SetSpriteCursorVisible(false);
    }
}

// =======================================================================
// SPRITE DEL CURSOR
// =======================================================================

void ASpaceCameraPawn::SetSpriteCursorVisible(bool bVisible)
{
    if (!SpriteCursorComp) return;

    const bool bTieneTextura = (SpriteCursorTexture != nullptr);

    SpriteCursorComp->SetVisibility(bVisible && bTieneTextura);
    SpriteCursorComp->SetHiddenInGame(!(bVisible && bTieneTextura));
}

void ASpaceCameraPawn::UpdateSpriteCursorPosition()
{
    if (!SpriteCursorComp) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    FVector WorldLocation, WorldDirection;
    if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        const FVector PosicionCursor = WorldLocation + (WorldDirection * TargetDistance);

        SpriteCursorComp->SetWorldLocation(PosicionCursor);
        SpriteCursorComp->SetWorldScale3D(SpriteCursorScale);
        SpriteCursorComp->SetWorldRotation(FRotator::ZeroRotator);
    }
}

// =======================================================================
// INPUT
// =======================================================================

void ASpaceCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent) return;

    PlayerInputComponent->BindAction("ClickLeft", IE_Pressed, this, &ASpaceCameraPawn::OnClickLeftPressed);
    PlayerInputComponent->BindAction("ClickLeft", IE_Released, this, &ASpaceCameraPawn::OnClickLeftReleased);

    PlayerInputComponent->BindAction("RotateCamera", IE_Pressed, this, &ASpaceCameraPawn::OnRightMousePressed);
    PlayerInputComponent->BindAction("RotateCamera", IE_Released, this, &ASpaceCameraPawn::OnRightMouseReleased);

    PlayerInputComponent->BindAction("ZoomIn", IE_Pressed, this, &ASpaceCameraPawn::OnZoomIn);
    PlayerInputComponent->BindAction("ZoomOut", IE_Pressed, this, &ASpaceCameraPawn::OnZoomOut);

    PlayerInputComponent->BindAxis("MouseX", this, &ASpaceCameraPawn::OnMouseX);
    PlayerInputComponent->BindAxis("MouseY", this, &ASpaceCameraPawn::OnMouseY);
}

// =======================================================================
// UTILIDADES DE MEDICIÓN
// =======================================================================

float ASpaceCameraPawn::GetRadioVisualObjeto(AActor* Actor) const
{
    if (!IsValid(Actor)) return 100.0f;

    static const FName NombresVariables[] = {
        FName("Get Star Radius"),
        FName("GetStarRadius"),
        FName("RadioEstrella"),
        FName("RadioVisual")
    };

    for (const FName& NombreVar : NombresVariables)
    {
        if (FProperty* Prop = Actor->GetClass()->FindPropertyByName(NombreVar))
        {
            if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
            {
                float ValorRadio = FloatProp->GetPropertyValue_InContainer(Actor);
                if (ValorRadio > 0.0f)
                {
                    return ValorRadio;
                }
            }
        }
    }

    if (UFunction* Func = Actor->FindFunction(FName("GetStarRadius")))
    {
        struct { float ReturnValue = 0.0f; } Params;
        Actor->ProcessEvent(Func, &Params);
        if (Params.ReturnValue > 0.0f)
        {
            return Params.ReturnValue;
        }
    }

    TArray<UActorComponent*> Componentes;
    Actor->GetComponents(Componentes);
    for (UActorComponent* Comp : Componentes)
    {
        if (Comp && Comp->GetName().Equals(TEXT("Folder"), ESearchCase::IgnoreCase))
        {
            if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp))
            {
                const float RadioFolder = PrimComp->Bounds.SphereRadius;
                if (RadioFolder > 0.0f)
                {
                    return RadioFolder;
                }
            }
        }
    }

    TArray<UStaticMeshComponent*> MeshComponents;
    Actor->GetComponents<UStaticMeshComponent>(MeshComponents);

    if (MeshComponents.Num() > 0)
    {
        FBox CombinedBox(ForceInit);
        for (UStaticMeshComponent* MeshComp : MeshComponents)
        {
            if (MeshComp && MeshComp->IsVisible())
            {
                CombinedBox += MeshComp->Bounds.GetBox();
            }
        }

        if (CombinedBox.IsValid)
        {
            return FMath::Max(100.0f, CombinedBox.GetExtent().GetMax());
        }
    }

    const float EscalaActor = Actor->GetActorScale3D().GetMax();
    return FMath::Max(100.0f, 500.0f * EscalaActor);
}

float ASpaceCameraPawn::GetMinDistanciaPermitida() const
{
    if (IsValid(TargetActor))
    {
        const float RadioObjeto = GetRadioVisualObjeto(TargetActor);
        if (RadioObjeto > 0.0f)
        {
            return RadioObjeto * MultiplicadorEncuadre;
        }
    }

    return (bUsarDistanciasPersonalizadas || !GI) ? MinDistance : GI->GetMinDistanceNivelActual();
}

// =======================================================================
// CLICK IZQUIERDO
// =======================================================================

void ASpaceCameraPawn::OnClickLeft()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    FHitResult HitResult;
    PC->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult);

    if (HitResult.bBlockingHit && IsValid(HitResult.GetActor()))
    {
        AActor* HitActor = HitResult.GetActor();

        if (HitActor->GetName().Contains(TEXT("Polvo")) || HitActor->ActorHasTag(TEXT("PolvoEstelar")))
        {
            // 👇 LEEMOS EL VALOR ANTES DE DESTRUIR
            int32 ValorPolvoObtenido = ObtenerValorPolvo(HitActor);

            HitActor->Destroy();

            if (GI)
            {
                GI->AddPolvoConSkills(ValorPolvoObtenido);
            }
        }

        bool bEsObjetoEspacial = HitActor->ActorHasTag(TEXT("Seleccionable"));
        if (!bEsObjetoEspacial)
        {
            UClass* ClassIter = HitActor->GetClass();
            while (ClassIter)
            {
                if (ClassIter->GetName().Contains(TEXT("ObjetoEspacial")))
                {
                    bEsObjetoEspacial = true;
                    break;
                }
                ClassIter = ClassIter->GetSuperClass();
            }
        }

        if (bEsObjetoEspacial)
        {
            if (TargetActor == HitActor && bMenuInferiorAbierto)
            {
                return;
            }

            TargetActor = HitActor;

            if (const AEstrellaActor* Estrella = Cast<AEstrellaActor>(HitActor))
            {
                SeleccionarEstrella(Estrella->TipoEstrella);
            }
            const float MaxDist = (bUsarDistanciasPersonalizadas || !GI) ? MaxDistance : GI->GetMaxDistanceNivelActual();

            if (float* SavedDist = StarMenuDistances.Find(TargetActor))
            {
                DistanciaCierreMenu = *SavedDist;
            }
            else
            {
                DistanciaCierreMenu = MaxDist;
                StarMenuDistances.Add(TargetActor, MaxDist);
            }

            if (bEncuadrarAlSeleccionar && IsValid(TargetActor))
            {
                const float RadioObjeto = GetRadioVisualObjeto(TargetActor);
                const float DistanciaDeseada = RadioObjeto * MultiplicadorEncuadre;
                const float MinDist = GetMinDistanciaPermitida();

                TargetDistance = FMath::Clamp(DistanciaDeseada, MinDist, MaxDist);
            }

            if (TargetDistance >= DistanciaCierreMenu)
            {
                TargetDistance = DistanciaCierreMenu;
            }

            if (HUDRef)
            {
                if (UFunction* FuncAbrir = HUDRef->FindFunction(FName("AbrirMenuInferior")))
                {
                    HUDRef->ProcessEvent(FuncAbrir, nullptr);
                    bMenuInferiorAbierto = true;
                }

                if (UFunction* FuncActualizar = HUDRef->FindFunction(FName("ActualizarEstadoBotones")))
                {
                    HUDRef->ProcessEvent(FuncActualizar, nullptr);
                }
            }
        }
    }
}

void ASpaceCameraPawn::OnClickLeftPressed()
{
    USkillsSubsystem* Skills = GI ? GI->GetSubsystem<USkillsSubsystem>() : nullptr;
    if (Skills && Skills->bAspiradoPolvoActivo)
    {
        bEstaArrastrandoPolvo = true;
        RadioAspiradoActual = Skills->RadioMinimo;
        PolvosDestruidosEnArrastre = 0;

        SetSpriteCursorVisible(true);
        UpdateSpriteCursorPosition();
    }

    OnClickLeft();
}

void ASpaceCameraPawn::OnClickLeftReleased()
{
    bEstaArrastrandoPolvo = false;

    USkillsSubsystem* Skills = GI ? GI->GetSubsystem<USkillsSubsystem>() : nullptr;
    if (Skills)
    {
        RadioAspiradoActual = Skills->RadioMinimo;
    }

    PolvosDestruidosEnArrastre = 0;
    SetSpriteCursorVisible(false);
}

// =======================================================================
// CLIC DERECHO - ROTACIÓN
// =======================================================================

void ASpaceCameraPawn::OnRightMousePressed() { bIsRightMousePressed = true; }
void ASpaceCameraPawn::OnRightMouseReleased() { bIsRightMousePressed = false; }

void ASpaceCameraPawn::OnMouseX(float AxisValue)
{
    if (bIsRightMousePressed && AxisValue != 0.0f)
    {
        AddControllerYawInput(AxisValue);
    }
}

void ASpaceCameraPawn::OnMouseY(float AxisValue)
{
    if (bIsRightMousePressed && AxisValue != 0.0f)
    {
        AddControllerPitchInput(AxisValue);
    }
}

// =======================================================================
// ZOOM
// =======================================================================

void ASpaceCameraPawn::OnZoomOut()
{
    const float MinDist = GetMinDistanciaPermitida();
    const float MaxDist = (bUsarDistanciasPersonalizadas || !GI) ? MaxDistance : GI->GetMaxDistanceNivelActual();

    const float DynamicStep = FMath::Max(StepZoomMinimo, TargetDistance * PorcentajeZoom);
    TargetDistance = FMath::Clamp(TargetDistance + DynamicStep, MinDist, MaxDist);

    if (IsValid(TargetActor))
    {
        StarMenuDistances.Add(TargetActor, MaxDist);
        DistanciaCierreMenu = MaxDist;
    }
}

void ASpaceCameraPawn::OnZoomIn()
{
    const float MinDist = GetMinDistanciaPermitida();
    const float MaxDist = (bUsarDistanciasPersonalizadas || !GI) ? MaxDistance : GI->GetMaxDistanceNivelActual();

    const float DynamicStep = FMath::Max(StepZoomMinimo, TargetDistance * PorcentajeZoom);
    TargetDistance = FMath::Clamp(TargetDistance - DynamicStep, MinDist, MaxDist);

    if (IsValid(TargetActor))
    {
        if (float* SavedDist = StarMenuDistances.Find(TargetActor))
        {
            DistanciaCierreMenu = *SavedDist;
        }
    }
}

// =======================================================================
// NIVEL DE CÁMARA
// =======================================================================

void ASpaceCameraPawn::AplicarNivelCamara()
{
    const float MinDist = (bUsarDistanciasPersonalizadas || !GI) ? MinDistance : GI->GetMinDistanceNivelActual();
    const float MaxDist = (bUsarDistanciasPersonalizadas || !GI) ? MaxDistance : GI->GetMaxDistanceNivelActual();

    TargetDistance = FMath::Clamp(TargetDistance, MinDist, MaxDist);
}

void ASpaceCameraPawn::HandleNivelCamaraCambiado(int32 NuevoNivel)
{
    // La cámara actual solo se actualizará cuando el jugador haga clic en otra estrella
}

// =======================================================================
// HABILIDAD DE ASPIRADO DE POLVO
// =======================================================================

void ASpaceCameraPawn::DesbloquearAspiradoPolvo()
{
    if (GI)
    {
        if (USkillsSubsystem* Skills = GI->GetSubsystem<USkillsSubsystem>())
        {
            Skills->bAspiradoPolvoActivo = true;
        }
    }
}

// =======================================================================
// EVOLUCIÓN Y SEGUIMIENTO DE NUEVA ESTRELLA
// =======================================================================

void ASpaceCameraPawn::FijarNuevaEstrellaEvolucionada(AActor* NuevaEstrella)
{
    if (!IsValid(NuevaEstrella)) return;

    TargetActor = NuevaEstrella;

    if (const AEstrellaActor* Estrella = Cast<AEstrellaActor>(NuevaEstrella))
    {
        
        SeleccionarEstrella(Estrella->TipoEstrella);
    }
    

    const float MaxDist = (bUsarDistanciasPersonalizadas || !GI) ? MaxDistance : GI->GetMaxDistanceNivelActual();

    if (bEncuadrarAlSeleccionar && IsValid(TargetActor))
    {
        const float RadioObjeto = GetRadioVisualObjeto(TargetActor);
        const float DistanciaDeseada = RadioObjeto * MultiplicadorEncuadre;
        const float MinDist = GetMinDistanciaPermitida();

        TargetDistance = FMath::Clamp(DistanciaDeseada, MinDist, MaxDist);
    }

    DistanciaCierreMenu = MaxDist;
    StarMenuDistances.Add(TargetActor, MaxDist);

    if (TargetDistance >= DistanciaCierreMenu)
    {
        TargetDistance = DistanciaCierreMenu;
    }

    if (HUDRef)
    {
        if (UFunction* FuncAbrir = HUDRef->FindFunction(FName("AbrirMenuInferior")))
        {
            HUDRef->ProcessEvent(FuncAbrir, nullptr);
            if (GI && GI->DebeMostrarContenidoEstrellas())
            {
                bMenuInferiorAbierto = true;
            }
        }

        // 👇 AÑADE ESTO NUEVO 👇
        if (UFunction* FuncActualizar = HUDRef->FindFunction(FName("ActualizarEstadoBotones")))
        {
            HUDRef->ProcessEvent(FuncActualizar, nullptr);
        }
        // 👆 AÑADE ESTO NUEVO 👆
    }
}

// =======================================================================
// MEJORAR ESTRELLA POR BOTÓN (1, 2 o 3)
// =======================================================================

void ASpaceCameraPawn::MejorarEstrellaBoton(int32 BotonIndex)
{
    if (!IsValid(TargetActor)) return;

    if (AEstrellaActor* EstrellaActual = Cast<AEstrellaActor>(TargetActor))
    {
        // 👇 NUEVO: En lugar de marcar solo el botón actual, marcamos todos los anteriores también.
        for (int32 i = 0; i <= BotonIndex; ++i)
        {
            switch (i)
            {
            case 0:  EstrellaActual->bBoton0Usado = true; break;
            case 1:  EstrellaActual->bBoton1Usado = true; break;
            case 2:  EstrellaActual->bBoton2Usado = true; break;
            case 3:  EstrellaActual->bBoton3Usado = true; break;
            case 4:  EstrellaActual->bBoton4Usado = true; break;
            case 5:  EstrellaActual->bBoton5Usado = true; break;
            case 6:  EstrellaActual->bBoton6Usado = true; break;
            case 7:  EstrellaActual->bBoton7Usado = true; break;
            case 8:  EstrellaActual->bBoton8Usado = true; break;
            case 9:  EstrellaActual->bBoton9Usado = true; break;
            case 10: EstrellaActual->bBoton10Usado = true; break;
            default: break;
            }
        }

        // Forzamos la etapa al índice del botón + 1 para mantener todo sincronizado
        EstrellaActual->EtapaEvolucion = BotonIndex + 1;
    }

    UEvolutionComponent* EvoComp = TargetActor->FindComponentByClass<UEvolutionComponent>();
    if (EvoComp)
    {
        EvoComp->IniciarEvolucionEnReversa();
    }

    if (HUDRef)
    {
        if (UFunction* FuncActualizar = HUDRef->FindFunction(FName("ActualizarEstadoBotones")))
        {
            HUDRef->ProcessEvent(FuncActualizar, nullptr);
        }
    }
}

bool ASpaceCameraPawn::ComprarEvolucionDestino(TSoftClassPtr<AActor> ClaseDestino, int32 IndiceTarjeta, int32 CostoPolvo)
{
    if (!IsValid(TargetActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Evo] RECHAZO 1: TargetActor no valido."));
        return false;
    }

    AEstrellaActor* EstrellaActual = Cast<AEstrellaActor>(TargetActor);
    if (!EstrellaActual)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Evo] RECHAZO 2: TargetActor (%s) no es AEstrellaActor."), *TargetActor->GetName());
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Evo] Estrella=%s Etapa=%d Tarjeta=%d Costo=%d"),
        *EstrellaActual->GetName(), EstrellaActual->EtapaEvolucion, IndiceTarjeta, CostoPolvo);

    if (EstrellaActual->EtapaEvolucion != IndiceTarjeta)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Evo] RECHAZO 3: etapa %d distinta de tarjeta %d."), EstrellaActual->EtapaEvolucion, IndiceTarjeta);
        return false;
    }

    UClass* ClaseCargada = ClaseDestino.LoadSynchronous();
    if (!ClaseCargada)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Evo] RECHAZO 4: ClaseDestino no carga (%s)."), *ClaseDestino.ToString());
        return false;
    }

    UEvolutionComponent* EvoComp = TargetActor->FindComponentByClass<UEvolutionComponent>();
    if (!EvoComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Evo] RECHAZO 5: la estrella no tiene EvolutionComponent."));
        return false;
    }

    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Evo] RECHAZO 6: GI es nulo."));
        return false;
    }

    if (GI->PolvoTotal < CostoPolvo)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Evo] RECHAZO 7: polvo %d menor que costo %d."), (int32)GI->PolvoTotal, CostoPolvo);
        return false;
    }

    GI->SetPolvoTotal(GI->PolvoTotal - CostoPolvo);

    GI->AlEvolucionarEstrella(EstrellaActual->TipoEstrella, IndiceTarjeta + 1);

    EvoComp->SuccessorStarClass = ClaseCargada;

    MejorarEstrellaBoton(IndiceTarjeta);

    UE_LOG(LogTemp, Warning, TEXT("[Evo] OK: evolucionando a %s."), *ClaseCargada->GetName());

    if (HUDRef)
    {
        if (UFunction* Func = HUDRef->FindFunction(FName("ActualizarEstadoBotones")))
        {
            HUDRef->ProcessEvent(Func, nullptr);
        }
    }
    return true;
}
void ASpaceCameraPawn::SeleccionarEstrella(int32 NivelEstrella)
{
    EstrellaSeleccionada = NivelEstrella;
    OnEstrellaSeleccionada.Broadcast(NivelEstrella);
}