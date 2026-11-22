// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UBPC_EfectoPolvo.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNIVERSO_API UBPC_EfectoPolvo : public UActorComponent
{
	GENERATED_BODY()

public:
	UBPC_EfectoPolvo();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Polvo | Animacion")
	void IniciarEfecto(const FVector& EnEscalaObjetivo);

	UFUNCTION(BlueprintCallable, Category = "Polvo | Animacion")
	void IniciarRetiro(float Duracion);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polvo | Animacion")
	float VelocidadEscalado = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polvo | Movimiento")
	float RadioMovimientoMaximo = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polvo | Movimiento")
	float VelocidadMovimiento = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Polvo | Movimiento")
	bool bEscalarMovimientoConTamano = true;

	// Se activa cuando el Agujero Negro lo atrapa para que deje de flotar
	UPROPERTY(BlueprintReadWrite, Category = "Polvo | Movimiento")
	bool bAtraidoPorAgujeroNegro = false;

	UFUNCTION(BlueprintCallable, Category = "Polvo | Animacion")
	void IniciarAbsorcion(float Duracion);

	UFUNCTION(BlueprintPure, Category = "Polvo | Animacion")
	bool EstaAbsorbido() const { return bAbsorbido; }

private:
	UPROPERTY()
	AActor* OwnerActor = nullptr;

	FVector PosicionOrigen = FVector::ZeroVector;
	FVector OffsetRelativo = FVector::ZeroVector;
	FVector EscalaObjetivo = FVector::OneVector;
	FVector OffsetsTiempo = FVector::ZeroVector;

	// Guarda el tamaño justo antes de empezar a retirarse
	FVector EscalaInicialRetiro = FVector::OneVector;

	float TiempoAcumulado = 0.0f;
	bool bEscaladoCompletado = false;
	bool bRetirando = false;

	float TiempoRetiroActual = 0.0f;
	float DuracionRetiro = 1.0f;
	bool bAbsorbido = false;
};