// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SkillsDatos.h"
#include "SkillsSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillComprada, FName, IdSkill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillsReseteadas);

UCLASS()
class UNIVERSO_API USkillsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ---------------- COMPRA Y CONSULTA ----------------

    UFUNCTION(BlueprintCallable, Category = "Skills")
    bool ComprarSkill(FName IdSkill);

    UFUNCTION(BlueprintPure, Category = "Skills")
    bool EstaSkillComprada(FName IdSkill) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    bool PuedeComprarSkill(FName IdSkill) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    int32 ObtenerCostoSkill(FName IdSkill) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    int32 ObtenerCantidadSkill(FName IdSkill) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    float ObtenerIntervaloSkill(FName IdSkill) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    int32 CalcularPrecioConDescuento(int32 PrecioOriginal) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    FText ObtenerTextoPrecioSkill(FName IdSkill) const;


    UFUNCTION(BlueprintPure, Category = "Skills|Aspirado")
    float ObtenerRadioMaximoAspirado() const;

    // 👇 NUEVAS FUNCIONES PARA NIVELES
    UFUNCTION(BlueprintPure, Category = "Skills") int32 ObtenerNivelSkill(FName IdSkill) const;
    UFUNCTION(BlueprintPure, Category = "Skills") int32 ObtenerNivelMaximoSkill(FName IdSkill) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    bool EsCinturonAsteroidesActivo() const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    float ObtenerPorcentajeCinturonAsteroides() const;

    // ---------------- EFECTOS ACTIVOS ----------------

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsPolvoFinoActivo() const;
    UFUNCTION(BlueprintPure, Category = "Skills") int32 ObtenerProbabilidadPolvoFino() const;

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsMagnetismoEstelarActivo() const;

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsSupernovaDormidaActiva() const;
    UFUNCTION(BlueprintPure, Category = "Skills") int32 ObtenerProbabilidadSupernova() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerDuracionSupernova() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerIntervaloSupernova() const;

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsNucleoCalienteActiva() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerPorcentajeNucleoCaliente() const;

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsEcoCosmicoActivo() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerPorcentajeEcoCosmico() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerDuracionEcoCosmico() const;

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsOrbitaEstableActiva() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerPorcentajeOrbitaEstable() const;

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsEstrellaDormidaActiva() const;
    UFUNCTION(BlueprintPure, Category = "Skills") int32 ObtenerProbabilidadEstrellaDormida() const;

    // ---------------- VARIABLES DE ASPIRADO ----------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills|Aspirado")
    bool bAspiradoPolvoActivo = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills|Aspirado")
    int32 NivelRecoleccion = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills|Aspirado")
    float RadioMinimo = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills|Aspirado")
    float RadioMaximo = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills|Aspirado")
    float VelocidadExpansion = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills|Aspirado")
    int32 MaxPolvoPorAccion = 3;

    // ---------------- REINICIO Y EVENTOS ----------------

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ResetearSkills();

    UFUNCTION(BlueprintPure, Category = "Skills")
    int32 ObtenerBonusTotal(ETipoEfectoSkill Tipo) const;

    UFUNCTION(BlueprintPure, Category = "Skills")
    int32 CalcularPolvoGanado(int32 PolvoBase) const;

    UPROPERTY(BlueprintAssignable, Category = "Skills")
    FOnSkillComprada OnSkillComprada;

    UPROPERTY(BlueprintAssignable, Category = "Skills")
    FOnSkillsReseteadas OnSkillsReseteadas;

    // ---------------- GUARDADO ----------------
    TArray<FName> ObtenerSkillsCompradas() const;
    void CargarSkillsCompradas(const TArray<FName>& Compradas);

    TMap<FName, int32> ObtenerNivelesSkills() const;
    void CargarNivelesSkills(const TMap<FName, int32>& Niveles);

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsNebulosaActivaComprada() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerPorcentajePolvoNebulosa() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerPorcentajePolvoFinoNebulosa() const;

    UFUNCTION(BlueprintPure, Category = "Skills") bool EsNebulosaSuperGalacticaActiva() const;
    UFUNCTION(BlueprintPure, Category = "Skills") float ObtenerProbabilidadEvolucionNebulosa() const;

private:
    UPROPERTY()
    TMap<FName, int32> NivelesActuales;

    const FSkillFila* BuscarFila(FName IdSkill) const;
};