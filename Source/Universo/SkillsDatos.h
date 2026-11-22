// Copyright (c) 2026 Universo. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SkillsDatos.generated.h"

UENUM(BlueprintType)
enum class ETipoEfectoSkill : uint8
{
    Ninguno                 UMETA(DisplayName = "Ninguno"),
    BonusPolvoPorClic       UMETA(DisplayName = "Bonus % de polvo por clic"),
    LluviaDePolvo           UMETA(DisplayName = "Lluvia de Polvo"),
    ReducirCostoEstrellas   UMETA(DisplayName = "Reducir Costo de Estrellas"),
    PolvoFino               UMETA(DisplayName = "Polvo Fino"),
    SupernovaDormida        UMETA(DisplayName = "Supernova Dormida"),
    ConstelacionI           UMETA(DisplayName = "Constelación I"),
    NucleoCaliente          UMETA(DisplayName = "Núcleo Caliente"),
    MagnetismoEstelar       UMETA(DisplayName = "Magnetismo Estelar"),
    EcoCosmico              UMETA(DisplayName = "Eco Cósmico"),
    DestruirPolvoAreas      UMETA(DisplayName = "Destruir Polvo en Áreas"), 
    OrbitaEstable           UMETA(DisplayName = "Órbita Estable"),
    EstrellaDormida         UMETA(DisplayName = "Estrella Dormida"),
    CinturonAsteroides      UMETA(DisplayName = "Cinturón de Asteroides"),
    NebulosaActiva          UMETA(DisplayName = "Nebulosa Activa"),
    NebulosaSuperGalactica  UMETA(DisplayName = "Nebulosa Super Galáctica"),
};

// 👇 NUEVO: La estructura que define cada nivel
USTRUCT(BlueprintType)
struct FSkillNivel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Costo = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 ValorPorcentaje = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 CantidadPolvo = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float IntervaloTiempo = 60.0f;
};

USTRUCT(BlueprintType)
struct FSkillFila : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText Nombre;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill", meta = (MultiLine = "true"))
    FText Descripcion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ETipoEfectoSkill TipoEfecto = ETipoEfectoSkill::Ninguno;

    // 👇 NUEVO: activa el modo incremental
  // 1.0  = desactivado  -> se usa el Costo de cada FSkillNivel (comportamiento actual)
  // 1.5  = +50 % por nivel -> Nivel2 = Base*1.5, Nivel3 = Base*1.5², ...
  // 2.0  = +100 % por nivel -> dobla cada nivel
  // 1.25 = +25 % por nivel (más suave)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float MultiplicadorCostoIncremental = 1.0f;

    // 👇 El array de niveles
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FSkillNivel> Niveles;
};