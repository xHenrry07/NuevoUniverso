// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "EstrellaActor.generated.h"

UCLASS()
class UNIVERSO_API AEstrellaActor : public AActor
{
    GENERATED_BODY()

public:
    AEstrellaActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Estrella|Componentes")
    UStaticMeshComponent* MeshEstrella;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bEsInamovible = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    int32 EtapaEvolucion = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    int32 TipoEstrella = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    int32 IDEstrella = 0;

    // 🚀 NUEVAS VARIABLES DE LINAJE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    TSubclassOf<AEstrellaActor> ClaseAnterior;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    TSubclassOf<AEstrellaActor> ClaseSiguiente;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bAnimacionInicialTerminada = false;

    // --- SKILL: ECO CÓSMICO ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Habilidades")
    bool bEcoCosmicoActivo = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Habilidades")
    float MultiplicadorEco = 1.0f;

    UFUNCTION(BlueprintCallable, Category = "Estrella|Habilidades")
    void ActivarEcoCosmico(float Porcentaje); // 👈 Ya no recibe duración

    UPROPERTY(BlueprintReadWrite, Category = "Estrella|Habilidades")
    bool bCinturonAsteroidesActivo = false;

    UPROPERTY(BlueprintReadWrite, Category = "Estrella|Habilidades")
    float MultiplicadorCinturon = 1.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Estrella|Habilidades")
    float TiempoRestanteCinturon = 0.0f;

    UFUNCTION(BlueprintCallable, Category = "Estrella|Habilidades")
    void ActivarCinturonAsteroides(float Porcentaje, float Duracion);

    // 👇 NUEVO: Para identificar la nebulosa
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Nebulosa")
    bool bEsNebulosa = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Nebulosa")
    float IntervaloPolvoPasivo = 5.0f; // Cada cuántos segundos da polvo

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Nebulosa")
    int32 PolvoBasePorNivel = 10; // Cuánto polvo da por cada nivel de evolución

    UFUNCTION(BlueprintCallable, Category = "Estrella|Evolucion")
    void EvolucionarAutomaticamente();

    // --- VARIABLES DE LOS BOTONES ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton0Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton1Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton2Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton3Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton4Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton5Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton6Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton7Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton8Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton9Usado = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bBoton10Usado = false;

    UPROPERTY(BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bCargadaDesdeSave = false;

    UPROPERTY(BlueprintReadWrite, Category = "Estrella|Evolucion")
    bool bEsEvolucion = false;

private:
    FTimerHandle TimerNebulosa;
    void GenerarPolvoPasivo();
};