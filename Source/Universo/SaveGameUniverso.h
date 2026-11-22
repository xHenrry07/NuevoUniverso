 // Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameUniverso.generated.h"

class AEstrellaActor;
class AActor;

USTRUCT(BlueprintType)
struct FEstrellaGuardada
{

    GENERATED_BODY()
    UPROPERTY()
    TSubclassOf<AEstrellaActor> ClaseEstrella;

    UPROPERTY()
    FTransform Transform;

    UPROPERTY()
    int32 EtapaEvolucion;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 TipoEstrella = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 IDEstrella = 0;

    // 👇 NUEVO: Guardar si ya nació
    UPROPERTY()
    bool bAnimacionInicialTerminada;

    UPROPERTY()
    bool bBoton0Usado;
    UPROPERTY()
    bool bBoton1Usado;
    UPROPERTY()
    bool bBoton2Usado;
    UPROPERTY()
    bool bBoton3Usado;
    UPROPERTY()
    bool bBoton4Usado;
    UPROPERTY()
    bool bBoton5Usado;
    UPROPERTY()
    bool bBoton6Usado;
    UPROPERTY()
    bool bBoton7Usado;
    UPROPERTY()
    bool bBoton8Usado;
    UPROPERTY()
    bool bBoton9Usado;
    UPROPERTY()
    bool bBoton10Usado;

    UPROPERTY()
    bool bEcoCosmicoActivo = false;

    UPROPERTY()
    float MultiplicadorEco = 1.0f; // 👈 ESTA ES LA QUE FALTA

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 EsenciaGuardada = 0;
};

USTRUCT(BlueprintType)
struct FPolvoGuardado
{
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<AActor> ClasePolvo;

    UPROPERTY()
    FTransform Transform;
};

UCLASS()
class UNIVERSO_API USaveGameUniverso : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 SiguienteIDEstrellaGuardado = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TMap<FName, int32> NivelesSkillsGuardados;

public:
    UPROPERTY()
    int32 PolvoTotalGuardado = 0;

    UPROPERTY()
    int32 NivelCamaraActualGuardado = 0;

    UPROPERTY()
    int32 ContadorEstrellasGuardado = 1;

    UPROPERTY()
    int32 EstrellasEvolucionadasGuardado = 0;

    UPROPERTY()
    float RadioExtraCreacionGuardado = 0.0f;

    // 👇 DATOS DE LA CÁMARA (MEJORADOS)
    UPROPERTY()
    FVector PosicionCamara;

    UPROPERTY()
    FRotator RotacionCamara;

    UPROPERTY()
    float DistanciaCamara = 2000.0f; // Distancia real del brazo

    UPROPERTY()
    float TargetDistanceGuardada = 2000.0f; // Distancia objetivo

    // 👇 NUEVO: Para saber si había una estrella seleccionada y su posición
    UPROPERTY()
    bool bTieneTargetGuardado = false;

    UPROPERTY()
    FVector TargetPosicionGuardada;

    // 👇 NUEVO: Para guardar la rotación exacta de la vista del jugador
    UPROPERTY()
    FRotator ControlRotationCamara;

    // Listas
    UPROPERTY()
    TArray<FEstrellaGuardada> EstrellasEnElMapa;

    UPROPERTY()
    TArray<FPolvoGuardado> PolvosEnElMapa;

    // 👇 NUEVO: Bandera para saber si ya hizo su animación de nacimiento
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bAnimacionInicialTerminada = false;
};