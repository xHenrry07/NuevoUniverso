// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BPC_BlackHole.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNIVERSO_API UBPC_BlackHole : public UActorComponent
{
    GENERATED_BODY()

public:
    UBPC_BlackHole();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

public:
    // 📏 Radio máximo de atracción (hasta dónde llega el imán)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole")
    float RadioAtraccion = 2000.0f;

    // 🚀 Velocidad a la que el polvo es atraído
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole")
    float VelocidadAtraccion = 5.0f;

    // 🕳️ Distancia a la que el polvo se considera "absorbido" y se destruye
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole")
    float DistanciaAbsorcion = 150.0f;

    // 🎨 Mostrar el radio dibujado en pantalla
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole|Debug")
    bool bMostrarRadio = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole|Debug")
    FColor ColorRadio = FColor::Purple;

    // Velocidad (uu/seg) en el borde del radio
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole")
    float VelocidadAtraccionMinima = 150.0f;

    // Velocidad (uu/seg) cuando está cerca del centro
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole")
    float VelocidadAtraccionMaxima = 1500.0f;

    // Cuánto dura el encogimiento del polvo al ser absorbido
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Black Hole")
    float DuracionAnimacionAbsorcion = 0.5f;

private:
    // Función auxiliar para saber cuánto vale el polvo que estamos absorbiendo
    int32 ObtenerValorPolvo(AActor* PolvoActor) const;
};