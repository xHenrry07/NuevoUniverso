#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BPC_CinturonAsteroides.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNIVERSO_API UBPC_CinturonAsteroides : public UActorComponent
{
    GENERATED_BODY()

public:
    UBPC_CinturonAsteroides();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- CONFIG (por estrella) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinturon")
    TSubclassOf<AActor> ClaseVisualCinturon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinturon")
    float RadioAtraccion = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinturon")
    float VelocidadAtraccion = 1500.0f;

    // 👇 AÑADE ESTA LÍNEA AQUÍ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinturon")
    float RadioCinturon = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinturon")
    float DistanciaAbsorcion = 200.0f;

    // Si true, este cinturón arranca solo (útil para testear en editor).
    // En producción déjalo FALSE: lo activa el GI al comprar la skill.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinturon")
    bool bAutoActivar = false;

    // --- ESTADO ---
    UPROPERTY(BlueprintReadOnly, Category = "Cinturon")
    bool bActivo = false;

    // --- API ---
    UFUNCTION(BlueprintCallable, Category = "Cinturon")
    void ActivarCinturon();

    UFUNCTION(BlueprintCallable, Category = "Cinturon")
    void DesactivarCinturon();

    UFUNCTION(BlueprintPure, Category = "Cinturon")
    bool EstaActivo() const { return bActivo; }

private:
    UPROPERTY()
    AActor* VisualSpawned = nullptr;

    int32 ObtenerValorPolvo(AActor* PolvoActor) const;
};