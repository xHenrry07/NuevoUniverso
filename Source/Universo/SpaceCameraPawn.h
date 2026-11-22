// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EvolutionComponent.h"
#include "SpaceCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UUGI_Espacio;
class UUserWidget;
class UBillboardComponent;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStarSelected, AActor*, Star, int32, StarLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEstrellaSeleccionada, int32, NivelEstrella);

UCLASS()
class UNIVERSO_API ASpaceCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    ASpaceCameraPawn();

    UFUNCTION(BlueprintCallable, Category = "Estrellas")
    void FijarNuevaEstrellaEvolucionada(AActor* NuevaEstrella);

    UPROPERTY(BlueprintAssignable, Category = "Estrellas")
    FOnEstrellaSeleccionada OnEstrellaSeleccionada;

    UPROPERTY(BlueprintReadOnly, Category = "Estrellas")
    int32 EstrellaSeleccionada = 0;

    UFUNCTION(BlueprintCallable, Category = "Estrellas")
    void SeleccionarEstrella(int32 NivelEstrella);

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Polvo")
    void DesbloquearAspiradoPolvo();

    // COMPONENTES
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camara")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camara")
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camara")
    TObjectPtr<UCameraComponent> Camera;

    // SPRITE 2D DEL CURSOR
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camara|Cursor")
    TObjectPtr<UBillboardComponent> SpriteCursorComp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Cursor")
    TObjectPtr<UTexture2D> SpriteCursorTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Cursor")
    FVector SpriteCursorScale = FVector(1.0f, 1.0f, 1.0f);

    // REFERENCIAS
    UPROPERTY()
    TObjectPtr<UUGI_Espacio> GI;

    // CONFIGURACIÓN DE CÁMARA
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Config")
    float InterpSpeed = 5.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Camara|Config")
    bool bGameLoaded = false;

    UPROPERTY(BlueprintReadWrite, Category = "Camara|UI")
    TObjectPtr<UUserWidget> HUDRef;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Seleccion")
    TObjectPtr<AActor> TargetActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Config")
    float TargetDistance = 2000.0f;

    // ESTADO DE POLVO
    bool bEstaArrastrandoPolvo = false;
    float RadioAspiradoActual = 100.0f;
    int32 PolvosDestruidosEnArrastre = 0;

    // ZOOM
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Zoom")
    float PorcentajeZoom = 0.15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Zoom")
    float StepZoomMinimo = 200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Zoom")
    float MargenSeguridadObjeto = 100.0f;

    // ENCUADRE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Encuadre")
    bool bEncuadrarAlSeleccionar = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Encuadre")
    float MultiplicadorEncuadre = 1.3f;

    // AUTO-CIERRE DE MENÚ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|UI")
    float DistanciaCierreMenu = 3000.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camara|UI")
    bool bMenuInferiorAbierto = false;

    // DISTANCIAS
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Distancia")
    bool bUsarDistanciasPersonalizadas = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Distancia")
    float MinDistance = 1000.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camara|Distancia")
    float MaxDistance = 10000.0f;

    UFUNCTION(BlueprintCallable, Category = "Estrellas")
    bool ComprarEvolucionDestino(TSoftClassPtr<AActor> ClaseDestino, int32 IndiceTarjeta, int32 CostoPolvo);

    UFUNCTION(BlueprintCallable, Category = "Estrella|Evolucion")
    void MejorarEstrellaBoton(int32 BotonIndex);

    UPROPERTY()
    TMap<TObjectPtr<AActor>, float> StarMenuDistances;

    int32 ObtenerValorPolvo(AActor* HitActor);

    // INPUT
    bool bIsRightMousePressed = false;
    void OnClickLeft();
    void OnClickLeftPressed();
    void OnClickLeftReleased();
    void OnRightMousePressed();
    void OnRightMouseReleased();
    void OnMouseX(float AxisValue);
    void OnMouseY(float AxisValue);
    void OnZoomIn();
    void OnZoomOut();

    UFUNCTION()
    void HandleNivelCamaraCambiado(int32 NuevoNivel);

    void AplicarNivelCamara();
    float GetMinDistanciaPermitida() const;
    float GetRadioVisualObjeto(AActor* Actor) const;
    void SetSpriteCursorVisible(bool bVisible);
    void UpdateSpriteCursorPosition();
};