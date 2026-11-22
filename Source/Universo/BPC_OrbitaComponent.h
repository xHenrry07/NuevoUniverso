// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "BPC_OrbitaComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNIVERSO_API UBPC_OrbitaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBPC_OrbitaComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // ---------------------------------------------------------------
    // CONFIGURACIÓN
    // ---------------------------------------------------------------

    /** Radio base de la órbita. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Órbita")
    float RadioOrbitaBase = 600.0f;

    /** Color con el que se dibuja la órbita en pantalla. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Órbita")
    FColor ColorOrbita = FColor::Cyan;

    /** Fuerza con la que se empuja al actor cuando siente una colisión con otra órbita. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Órbita")
    float FuerzaRepulsion = 1000.0f;

    /** Si está activo, se dibuja la órbita (útil para debug). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Órbita")
    bool bMostrarOrbita = true;


    /**
     * Si está a true, este actor NO se moverá cuando sienta una colisión.
     * Útil para la estrella que está creciendo (ancla) o para objetos fijos.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Órbita")
    bool bEstaCreciendo = false;

    // ---------------------------------------------------------------
    // API
    // ---------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Órbita")
    float GetRadioOrbitaEfectivo() const;

    UFUNCTION(BlueprintCallable, Category = "Órbita")
    static float GetRadioOrbitaDeClase(TSubclassOf<AActor> ActorClass);

    /**
     * Encuentra una posición válida para spawnear un nuevo actor.
     * Comprueba contra TODOS los actores existentes en el mundo que tengan
     * un componente UBPC_OrbitaComponent.
     */
    UFUNCTION(BlueprintCallable, Category = "Órbita", meta = (WorldContext = "WorldContextObject"))
    static bool EncontrarPosicionOrbitalValida(
        UObject* WorldContextObject,
        float MinRadius,
        float MaxRadius,
        float MinSeparation,
        int32 MaxAttempts,
        FVector& OutLocation
    );

    /** Registra este actor en el sistema orbital (por ahora no hace nada, reservado para futuro). */
    UFUNCTION(BlueprintCallable, Category = "Órbita")
    void RegistrarEnSistemaOrbital();

private:
    UPROPERTY()
    USphereComponent* OrbitCollider;

    void AplicarRepulsionOrbital(float DeltaTime);
    void DibujarOrbitaVisual();
};