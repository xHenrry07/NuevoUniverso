#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EvolutionComponent.generated.h"

UENUM(BlueprintType)
enum class EEvolutionAnimState : uint8
{
    None,
    Spawning,   // Animaci�n de aparici�n (De 0 a InitialScaleValue)
    Shrinking   // Animaci�n de evoluci�n (De InitialScaleValue a FinalScaleValue)
};

// Declaraci�n del delegado para avisar al Blueprint cuando termina el nacimiento/spawn
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpawnFinishedDelegate);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNIVERSO_API UEvolutionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEvolutionComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- CONFIGURACI�N DESDE EL EDITOR ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    TSubclassOf<AActor> SuccessorStarClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    class UCurveFloat* AnimationCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    float AnimationDuration = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    float SpawnAnimationDuration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    FName NiagaraParamName = FName("Scale");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    float SpawnStartScale = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    float InitialScaleValue = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolucion")
    float FinalScaleValue = 0.0f;

    // --- EVENTO / DELEGADO PARA EL BLUEPRINT ---
    UPROPERTY(BlueprintAssignable, Category = "Evolucion")
    FOnSpawnFinishedDelegate OnSpawnFinished;

    // --- FUNCIONES ---
    UFUNCTION(BlueprintCallable, Category = "Evolucion")
    void IniciarEvolucionEnReversa();

    UFUNCTION(BlueprintCallable, Category = "Evolucion")
    void IniciarSpawnAnimation();

private:
    EEvolutionAnimState AnimState = EEvolutionAnimState::None;
    float CurrentAnimationTime = 0.0f;
    float CurrentStartScaleForAnim = 0.0f;
    float CurrentTargetScaleForAnim = 0.0f;
    float CurrentActiveDuration = 1.5f;

    bool bPendienteEmitirSpawnFinished = false;

    void FinalizarEvolucion();
    void SetNiagaraScale(float Scale);
};