// Copyright (c) 2026 Universo. All Rights Reserved.

#include "UBPC_EfectoPolvo.h"
#include "GameFramework/Actor.h"

UBPC_EfectoPolvo::UBPC_EfectoPolvo()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBPC_EfectoPolvo::BeginPlay()
{
	Super::BeginPlay();

	OwnerActor = GetOwner();
	if (OwnerActor)
	{
		PosicionOrigen = OwnerActor->GetActorLocation();

		if (AActor* Parent = OwnerActor->GetOwner())
		{
			OffsetRelativo = PosicionOrigen - Parent->GetActorLocation();
		}

		OffsetsTiempo = FVector(
			FMath::FRandRange(0.0f, 100.0f),
			FMath::FRandRange(0.0f, 100.0f),
			FMath::FRandRange(0.0f, 100.0f)
		);
	}
}

void UBPC_EfectoPolvo::IniciarEfecto(const FVector& EnEscalaObjetivo)
{
	EscalaObjetivo = EnEscalaObjetivo;
	bEscaladoCompletado = false;
	bRetirando = false;
	bAbsorbido = false;
	bAtraidoPorAgujeroNegro = false;

	if (OwnerActor)
	{
		PosicionOrigen = OwnerActor->GetActorLocation();
		OwnerActor->SetActorScale3D(FVector::ZeroVector);
	}
}

void UBPC_EfectoPolvo::IniciarRetiro(float Duracion)
{
	bRetirando = true;
	DuracionRetiro = FMath::Max(Duracion, 0.01f);
	TiempoRetiroActual = 0.0f;

	// Guardamos el tamaño actual para interpolarlo de forma lineal
	if (OwnerActor)
	{
		EscalaInicialRetiro = OwnerActor->GetActorScale3D();
	}
}

void UBPC_EfectoPolvo::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerActor) return;

	// Lógica de retiro / absorción temporizado
	if (bRetirando)
	{
		TiempoRetiroActual += DeltaTime;
		float Alpha = FMath::Clamp(TiempoRetiroActual / DuracionRetiro, 0.0f, 1.0f);

		// Interpolamos desde la EscalaInicialRetiro
		OwnerActor->SetActorScale3D(FMath::Lerp(EscalaInicialRetiro, FVector::ZeroVector, Alpha));

		if (Alpha >= 1.0f)
		{
			OwnerActor->Destroy();
		}
		return;
	}

	// --- Lógica normal de nacimiento y flotación ---
	TiempoAcumulado += DeltaTime * VelocidadMovimiento;

	if (!bEscaladoCompletado)
	{
		const FVector EscalaActual = OwnerActor->GetActorScale3D();
		const FVector NuevaEscala = FMath::VInterpTo(EscalaActual, EscalaObjetivo, DeltaTime, VelocidadEscalado);
		OwnerActor->SetActorScale3D(NuevaEscala);

		if (FVector::DistSquared(NuevaEscala, EscalaObjetivo) < 0.001f)
		{
			OwnerActor->SetActorScale3D(EscalaObjetivo);
			bEscaladoCompletado = true;
		}
	}

	// Si un agujero negro lo arrastra, seguimos creciendo pero NO tocamos su posición
	if (bAtraidoPorAgujeroNegro)
	{
		return;
	}

	float MultiplicadorEscala = 1.0f;
	if (bEscalarMovimientoConTamano)
	{
		MultiplicadorEscala = FMath::Max(OwnerActor->GetActorScale3D().GetMax(), 0.01f);
	}

	const float RadioEfectivo = RadioMovimientoMaximo * MultiplicadorEscala;

	const float OffsetX = FMath::Sin(TiempoAcumulado + OffsetsTiempo.X) * RadioEfectivo;
	const float OffsetY = FMath::Cos(TiempoAcumulado * 0.8f + OffsetsTiempo.Y) * RadioEfectivo;
	const float OffsetZ = FMath::Sin(TiempoAcumulado * 1.2f + OffsetsTiempo.Z) * (RadioEfectivo * 0.5f);

	FVector NuevaBase = PosicionOrigen;
	if (AActor* Parent = OwnerActor->GetOwner())
	{
		NuevaBase = Parent->GetActorLocation() + OffsetRelativo;
	}

	OwnerActor->SetActorLocation(NuevaBase + FVector(OffsetX, OffsetY, OffsetZ));
}

void UBPC_EfectoPolvo::IniciarAbsorcion(float Duracion)
{
	bAbsorbido = true;
	bAtraidoPorAgujeroNegro = true;
	IniciarRetiro(Duracion); // reutiliza el encogimiento 1 -> 0 y el Destroy final
}

