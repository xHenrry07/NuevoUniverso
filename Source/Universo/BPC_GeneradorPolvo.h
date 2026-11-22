#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "BPC_GeneradorPolvo.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPolvoDestruido, AActor*, PolvoDestruido);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNIVERSO_API UBPC_GeneradorPolvo : public UActorComponent
{
    GENERATED_BODY()

public:
    UBPC_GeneradorPolvo();

protected:
    virtual void BeginPlay() override;

public:
    // Inicia la generación de polvo manualmente (llamable desde Blueprint al terminar una animación)
    UFUNCTION(BlueprintCallable, Category = "Generador Polvo")
    void IniciarGeneracionPolvo();

    // Si está en true, arranca solo en BeginPlay. Si está en false, espera a que lo llames manualmente.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Spawn")
    bool bAutoStart = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Spawn")
    TSubclassOf<AActor> ClasePolvoEstelar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Spawn")
    int32 MaxPolvo = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Spawn")
    float MinRadius = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Spawn")
    float MaxRadius = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Spawn")
    float MinDistanciaPolvo = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Tiempo")
    float MinSpawnTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Tiempo")
    float MaxSpawnTime = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Escala")
    float MinScale = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Escala")
    float MaxScale = 1.5f;

    // NUEVA VARIABLE: Cantidad de polvo que otorga cada partícula generada por este componente
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Economia")
    int32 ValorPolvo = 1;

    UPROPERTY(BlueprintAssignable, Category = "Eventos")
    FOnPolvoDestruido OnPolvoDestruido;


    // Cambia la función por esta (sin parámetros):
    UFUNCTION(BlueprintCallable, Category = "Generador Polvo")
    void RetirarTodoElPolvo();

    // --- NUEVA VARIABLE ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Animacion")
    float DuracionRetiroPolvo = 1.5f;


    // 👇 NUEVA FUNCIÓN: Para detener la creación de polvo
    UFUNCTION(BlueprintCallable, Category = "Generador Polvo")
    void DetenerGeneracionPolvo();

    // 👇 NUEVO: Registrar un polvo ya existente en el mundo (ej. cargado desde savegame)
// para que sea rastreado por RetirarTodoElPolvo y se cuente en MaxPolvo.
    UFUNCTION(BlueprintCallable, Category = "Generador Polvo")
    void RegistrarPolvoExistente(AActor* Polvo);

    UFUNCTION(BlueprintPure, Category = "Generador Polvo")
    int32 ObtenerValorDePolvo(AActor* Polvo) const;

    UFUNCTION(BlueprintCallable, Category = "Generador Polvo")
    void RegistrarValorPolvo(AActor* Polvo, int32 Valor);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Polvo Fino")
    bool bPermitePolvoFino = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Polvo Fino")
    TSubclassOf<AActor> ClasePolvoFino;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion | Polvo Fino")
    int32 MultiplicadorValorPolvoFino = 10;

    void VerificarMagnetismoEstelar();

    UFUNCTION()
    void OnSkillCompradaRecibida(FName IdSkill);

private:
    void EjecutarCicloSpawn();
    void CalcularUbicacionCandidata();
    bool ValidarPosicion();
    void SpawnearPolvo();
    void ProgramarSiguienteSpawn();

    UFUNCTION()
    void AlDestruirPolvo(AActor* DestroyedActor);

    FTimerHandle TimerHandle_Spawn;

    UPROPERTY()
    AActor* OwnerActor = nullptr;

    UPROPERTY()
    TArray<AActor*> PolvosCreados;

    int32 PolvoActual = 0;
    FTransform UbicacionCandidata;
    bool bPosicionValida = false;

    FTimerHandle TimerHandle_Magnetismo;
    void EjecutarMagnetismoEstelar();

    UPROPERTY()
    TMap<AActor*, int32> ValoresPolvoPersonalizados;

    bool bSupernovaActiva = false;
    FTimerHandle TimerHandle_SupernovaIntervalo;
    FTimerHandle TimerHandle_SupernovaDuracion;

    void VerificarSupernovaDormida();
    void IntentarActivarSupernova();
    void ActivarSupernova(float Duracion);
    void DesactivarSupernova();
};