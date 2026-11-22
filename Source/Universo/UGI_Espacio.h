// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UGI_Espacio.generated.h"


class AEstrellaActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNivelCamaraCambiado, int32, NuevoNivel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPolvoCambiado, int32, NuevoPolvo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLluviaDisparada, int32, CantidadOtorgada);

UCLASS(BlueprintType, Blueprintable)
class UNIVERSO_API UUGI_Espacio : public UGameInstance
{
    GENERATED_BODY()

public:
    UUGI_Espacio();

    virtual void Init() override;

    // El C++ avisa; el Blueprint (BP_GameMode_Universo) suma la esencia
    UFUNCTION(BlueprintImplementableEvent, Category = "Economia")
    void AlEvolucionarEstrella(int32 TipoEstrella, int32 EtapaNueva);

    // -------------------------------------------------------------------
    // SISTEMA DE GUARDADO
    // -------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardado")
    FString NombreSlotGuardado = TEXT("PartidaUniverso");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardado")
    float TiempoAutoguardado = 120.0f; // Cada 2 minutos

    UFUNCTION(BlueprintCallable, Category = "Guardado")
    void GuardarPartida();

    UFUNCTION(BlueprintCallable, Category = "Guardado")
    void CargarPartida();

    UFUNCTION(BlueprintCallable, Category = "Guardado")
    bool ExistePartidaGuardada() const;

    // dentro de UUGI_Espacio
    UFUNCTION(BlueprintCallable, Category = "Espacio|Cinturon")
    void ActivarCinturonEnEstrellaAleatoria();

    // -------------------------------------------------------------------
    // Nivel de cámara y Distancias
    // -------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Camara")
    int32 NivelCamaraActual = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Camara")
    int32 NivelCamaraMaximo = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Camara")
    TArray<float> DistanciasMinimasPorNivel = { 1000.0f, 5000.0f, 18000.0f, 50000.0f, 500000.0f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Camara")
    TArray<float> DistanciasMaximasPorNivel = { 1000.0f, 10000.0f, 50000.0f, 500000.0f, 1000000.0f };

    UPROPERTY(BlueprintAssignable, Category = "Espacio|Camara")
    FOnNivelCamaraCambiado OnNivelCamaraCambiado;

    // -------------------------------------------------------------------
    // Radios de Creación de Objetos por Nivel
    // -------------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Radio")
    TArray<float> RadiosMinimosPorNivel = { 100.0f, 100.0f, 100.0f, 100.0f, 100.0f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Radio")
    TArray<float> RadiosMaximosPorNivel = { 1000.0f, 5000.0f, 10000.0f, 50000.0f, 100000.0f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Radio")
    float RadioExtraCreacion = 0.0f;

    // API CÁMARA
    UFUNCTION(BlueprintCallable, Category = "Espacio|Camara")
    bool AvanzarNivelCamara();

    UFUNCTION(BlueprintCallable, Category = "Espacio|Camara")
    float GetMinDistanceNivelActual() const;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Camara")
    float GetMaxDistanceNivelActual() const;

    // API RADIOS
    UFUNCTION(BlueprintCallable, Category = "Espacio|Radio")
    float GetMinRadiusNivelActual() const;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Radio")
    float GetMaxRadiusNivelActual() const;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Radio")
    void AgregarRadioExtra(float CantidadExtra);

    // ESTRELLAS
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Estrellas")
    int32 ContadorEstrellas = 1;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Estrellas")
    void IncrementarEstrellas(int32 Cantidad = 1);

    UFUNCTION(BlueprintCallable, Category = "Espacio|Estrellas")
    bool DebeMostrarContenidoEstrellas() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Estrellas")
    int32 EstrellasEvolucionadas = 0;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Estrellas")
    void IncrementarEstrellasEvolucionadas(int32 Cantidad = 1);

    // 👇 Porcentaje de polvo que se convertirá en Esencia (ej. 10.0 = 10%)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economia|Reset")
    float PorcentajeEsenciaGalactica = 10.0f;

    // 👇 Función que se llamará desde el botón de tu UI
    UFUNCTION(BlueprintCallable, Category = "Guardado")
    void EjecutarResetGalactico();

    // 👇 Evento que el Blueprint (GameMode) escuchará para sumar la Esencia
    UFUNCTION(BlueprintImplementableEvent, Category = "Economia|Reset")
    void OnResetGalactico(int32 EsenciaGanada);



    // IDs de estrellas y Núcleo Caliente
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Estrellas")
    int32 SiguienteIDEstrella = 1;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Estrellas")
    int32 ObtenerSiguienteIDEstrella();

    UFUNCTION(BlueprintCallable, Category = "Espacio|Estrellas")
    bool EsEstrellaAntigua(AActor* Estrella);

    // RECURSOS
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Recursos")
    int32 PolvoTotal = 0;

    // Multiplicador de polvo por click (lo cambia la habilidad Universo Joven desde Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economia")
    float MultiplicadorProduccion = 1.0f;


    UPROPERTY(BlueprintAssignable, Category = "Espacio|Recursos")
    FOnPolvoCambiado OnPolvoCambiado;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Recursos")
    void AddPolvo(int32 Cantidad);

    UFUNCTION(BlueprintCallable, Category = "Espacio|Recursos")
    void SetPolvoTotal(int32 NuevoPolvo);

    UFUNCTION(BlueprintCallable, Category = "Espacio|Recursos")
    void NotificarCambioPolvo();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    TObjectPtr<class UDataTable> TablaSkills;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Recursos")
    void AddPolvoConSkills(int32 PolvoBase);


    UFUNCTION(BlueprintPure, Category = "Espacio|Nebulosas")
    int32 ObtenerCantidadNebulosas() const;

    UFUNCTION(BlueprintCallable, Category = "Espacio|Recursos")
    void AddPolvoPasivo(int32 PolvoBase);

    // ===================================================================
    // RESET DE PARTIDA
    // ===================================================================

    /**
     * Resetea toda la partida al estado inicial.
     * @param bBorrarGuardado          - Si true, elimina el archivo de guardado del disco.
     * @param bSpawnearEstrellaInicial - Si true, respawnea ClaseEstrellaInicial en el origen.
     */
    UFUNCTION(BlueprintCallable, Category = "Guardado")
    void ResetearPartida(bool bBorrarGuardado = true, bool bSpawnearEstrellaInicial = true);

    /** Clase de la estrella con la que arranca el juego (para respawnearla en el reset). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardado")
    TSubclassOf<AEstrellaActor> ClaseEstrellaInicial;

    /** Ubicación donde aparecerá la estrella inicial tras el reset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guardado")
    FVector UbicacionEstrellaInicial = FVector::ZeroVector;

    // -------------------------------------------------------------------
    // SKILL: LLUVIA DE POLVO
    // -------------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "Espacio|Lluvia")
    FOnLluviaDisparada OnLluviaDisparada;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Lluvia")
    bool bLluviaDePolvoActiva = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Espacio|Lluvia")
    TSubclassOf<AActor> ClaseVisualLluvia; // 👈 El BP_VisualLluvia que acabas de crear

    UFUNCTION(BlueprintCallable, Category = "Espacio|Lluvia")
    void InicializarLluviaDePolvoSiCorresponde();


    // -------------------------------------------------------------------
    // SKILL: CONSTELACIÓN I
    // -------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Espacio|Constelacion")
    void InicializarConstelacionISiCorresponde();

    //Estrella Dormida

    UFUNCTION(BlueprintCallable, Category = "Espacio|EstrellaDormida")
    void InicializarEstrellaDormidaSiCorresponde();

    UFUNCTION(BlueprintCallable, Category = "Espacio|NebulosaSuperGalactica")
    void InicializarNebulosaSuperGalacticaSiCorresponde();


private:
    FTimerHandle TimerAutoguardado;
    void AutoguardadoTick();

    // Lluvia de Polvo
    FTimerHandle TimerIntervaloLluvia;
    void EjecutarLluviaDePolvo();

    // Constelación I
    FTimerHandle TimerIntervaloConstelacion;
    void EjecutarConstelacionI();

    // Timer para Estrella Dormida
    FTimerHandle TimerIntervaloEstrellaDormida;
    void EjecutarEstrellaDormida();

    // Para escuchar cuando compras la skill
    UFUNCTION()
    void OnSkillCompradaRecibida(FName IdSkill);

    // Fracción de polvo acumulada por el multiplicador (para no perderla al redondear)
    float RestoPolvoFraccion = 0.0f;

    FTimerHandle TimerNebulosaSuperGalactica;
    void EjecutarEvolucionNebulosa();
};