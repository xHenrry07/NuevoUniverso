// Copyright (c) 2026 Universo. All Rights Reserved.

#include "SkillsSubsystem.h"
#include "UGI_Espacio.h"
#include "Engine/DataTable.h"

const FSkillFila* USkillsSubsystem::BuscarFila(FName IdSkill) const
{
    const UUGI_Espacio* GI = Cast<UUGI_Espacio>(GetGameInstance());
    if (!GI || !GI->TablaSkills || IdSkill.IsNone()) return nullptr;
    return GI->TablaSkills->FindRow<FSkillFila>(IdSkill, TEXT("USkillsSubsystem::BuscarFila"));
}

// ---------------- COMPRA Y CONSULTA ----------------

bool USkillsSubsystem::EstaSkillComprada(FName IdSkill) const
{
    const int32* Nivel = NivelesActuales.Find(IdSkill);
    return Nivel && *Nivel > 0;
}

int32 USkillsSubsystem::ObtenerNivelSkill(FName IdSkill) const
{
    const int32* Nivel = NivelesActuales.Find(IdSkill);
    return Nivel ? *Nivel : 0;
}

int32 USkillsSubsystem::ObtenerNivelMaximoSkill(FName IdSkill) const
{
    const FSkillFila* Fila = BuscarFila(IdSkill);
    return Fila ? Fila->Niveles.Num() : 0;
}

int32 USkillsSubsystem::ObtenerCostoSkill(FName IdSkill) const
{
    const FSkillFila* Fila = BuscarFila(IdSkill);
    if (!Fila || Fila->Niveles.Num() == 0) return 0;

    const int32 NivelActual = ObtenerNivelSkill(IdSkill);

    // Ya está al máximo
    if (NivelActual >= Fila->Niveles.Num()) return -1;

    // --- MODO INCREMENTAL ---
    if (Fila->MultiplicadorCostoIncremental > 1.0f)
    {
        // El coste del nivel 1 es la base
        const double CostoBase = static_cast<double>(Fila->Niveles[0].Costo);

        // Interés compuesto: Base * Mult^NivelActual
        const double CostoDouble =
            CostoBase * FMath::Pow(Fila->MultiplicadorCostoIncremental, NivelActual);

        // Protección contra overflow con niveles muy altos
        const double CostoClamped = FMath::Min(
            CostoDouble,
            static_cast<double>(MAX_int32 / 2));

        return FMath::Max(1, FMath::RoundToInt(CostoClamped));
    }

    return Fila->Niveles[NivelActual].Costo;
}

bool USkillsSubsystem::PuedeComprarSkill(FName IdSkill) const
{
    const int32 Costo = ObtenerCostoSkill(IdSkill);
    if (Costo < 0) return false; // al máximo

    const UUGI_Espacio* GI = Cast<UUGI_Espacio>(GetGameInstance());
    return GI && GI->PolvoTotal >= Costo;
}

bool USkillsSubsystem::ComprarSkill(FName IdSkill)
{
    if (!PuedeComprarSkill(IdSkill)) return false;

    UUGI_Espacio* GI = Cast<UUGI_Espacio>(GetGameInstance());
    const FSkillFila* Fila = BuscarFila(IdSkill);
    if (!GI || !Fila) return false;

    const int32 NivelActual = ObtenerNivelSkill(IdSkill);
    const int32 Costo = ObtenerCostoSkill(IdSkill);
    if (Costo < 0) return false;

    GI->SetPolvoTotal(GI->PolvoTotal - Costo);
    NivelesActuales.Add(IdSkill, NivelActual + 1);

    if (Fila->TipoEfecto == ETipoEfectoSkill::DestruirPolvoAreas)
    {
        bAspiradoPolvoActivo = true;
    }

    OnSkillComprada.Broadcast(IdSkill);
    return true;
}

void USkillsSubsystem::ResetearSkills()
{
    NivelesActuales.Empty();
    bAspiradoPolvoActivo = false;
    OnSkillsReseteadas.Broadcast();
}

int32 USkillsSubsystem::ObtenerBonusTotal(ETipoEfectoSkill Tipo) const
{
    int32 Total = 0;
    for (const auto& Elem : NivelesActuales)
    {
        const FSkillFila* Fila = BuscarFila(Elem.Key);
        if (Fila && Fila->TipoEfecto == Tipo && Elem.Value > 0 && Elem.Value <= Fila->Niveles.Num())
        {
            Total += Fila->Niveles[Elem.Value - 1].ValorPorcentaje;
        }
    }
    return Total;
}

int32 USkillsSubsystem::CalcularPolvoGanado(int32 PolvoBase) const
{
    const int32 Bonus = ObtenerBonusTotal(ETipoEfectoSkill::BonusPolvoPorClic);
    if (Bonus <= 0 || PolvoBase <= 0) return PolvoBase;
    const int64 Total = (static_cast<int64>(PolvoBase) * (100 + Bonus) + 50) / 100;
    return static_cast<int32>(FMath::Min<int64>(Total, MAX_int32));
}

int32 USkillsSubsystem::ObtenerCantidadSkill(FName IdSkill) const
{
    const FSkillFila* Fila = BuscarFila(IdSkill);
    int32 Nivel = ObtenerNivelSkill(IdSkill);
    if (Fila && Nivel > 0 && Nivel <= Fila->Niveles.Num()) return Fila->Niveles[Nivel - 1].CantidadPolvo;
    return 0;
}

float USkillsSubsystem::ObtenerIntervaloSkill(FName IdSkill) const
{
    const FSkillFila* Fila = BuscarFila(IdSkill);
    int32 Nivel = ObtenerNivelSkill(IdSkill);
    if (Fila && Nivel > 0 && Nivel <= Fila->Niveles.Num()) return Fila->Niveles[Nivel - 1].IntervaloTiempo;
    return 60.0f;
}

int32 USkillsSubsystem::CalcularPrecioConDescuento(int32 PrecioOriginal) const
{
    const int32 DescuentoPorcentaje = ObtenerBonusTotal(ETipoEfectoSkill::ReducirCostoEstrellas);
    if (DescuentoPorcentaje <= 0 || PrecioOriginal <= 0) return PrecioOriginal;
    const float Multiplicador = 1.0f - (static_cast<float>(DescuentoPorcentaje) / 100.0f);
    return FMath::Max(1, FMath::RoundToInt(static_cast<float>(PrecioOriginal) * Multiplicador));
}

// ---------------- FUNCIONES ESPECÍFICAS ----------------

bool USkillsSubsystem::EsPolvoFinoActivo() const { return EstaSkillComprada(FName("PolvoFino")); }
int32 USkillsSubsystem::ObtenerProbabilidadPolvoFino() const { return ObtenerBonusTotal(ETipoEfectoSkill::PolvoFino); }

bool USkillsSubsystem::EsMagnetismoEstelarActivo() const { return EstaSkillComprada(FName("MagnetismoEstelar")); }

bool USkillsSubsystem::EsSupernovaDormidaActiva() const { return EstaSkillComprada(FName("SupernovaDormida")); }
int32 USkillsSubsystem::ObtenerProbabilidadSupernova() const { return ObtenerBonusTotal(ETipoEfectoSkill::SupernovaDormida); }
float USkillsSubsystem::ObtenerDuracionSupernova() const { return (float)ObtenerCantidadSkill(FName("SupernovaDormida")); }
float USkillsSubsystem::ObtenerIntervaloSupernova() const { return ObtenerIntervaloSkill(FName("SupernovaDormida")); }

bool USkillsSubsystem::EsNucleoCalienteActiva() const { return EstaSkillComprada(FName("NucleoCaliente")); }
float USkillsSubsystem::ObtenerPorcentajeNucleoCaliente() const { return (float)ObtenerBonusTotal(ETipoEfectoSkill::NucleoCaliente); }

bool USkillsSubsystem::EsEcoCosmicoActivo() const { return EstaSkillComprada(FName("EcoCosmico")); }
float USkillsSubsystem::ObtenerPorcentajeEcoCosmico() const { return (float)ObtenerBonusTotal(ETipoEfectoSkill::EcoCosmico); }
float USkillsSubsystem::ObtenerDuracionEcoCosmico() const { return (float)ObtenerCantidadSkill(FName("EcoCosmico")); }

bool USkillsSubsystem::EsOrbitaEstableActiva() const { return EstaSkillComprada(FName("OrbitaEstable")); }
float USkillsSubsystem::ObtenerPorcentajeOrbitaEstable() const { return (float)ObtenerBonusTotal(ETipoEfectoSkill::OrbitaEstable); }

bool USkillsSubsystem::EsEstrellaDormidaActiva() const { return EstaSkillComprada(FName("EstrellaDormida")); }
int32 USkillsSubsystem::ObtenerProbabilidadEstrellaDormida() const { return ObtenerBonusTotal(ETipoEfectoSkill::EstrellaDormida); }

// ---------------- CINTURÓN DE ASTEROIDES ----------------
// 👇 SOLO UNA VEZ, SIN DUPLICADOS
bool USkillsSubsystem::EsCinturonAsteroidesActivo() const
{
    return EstaSkillComprada(FName("CinturonAsteroides"));
}

float USkillsSubsystem::ObtenerPorcentajeCinturonAsteroides() const
{
    return (float)ObtenerBonusTotal(ETipoEfectoSkill::CinturonAsteroides);
}

// ---------------- GUARDADO ----------------

TArray<FName> USkillsSubsystem::ObtenerSkillsCompradas() const
{
    TArray<FName> Keys;
    NivelesActuales.GetKeys(Keys);
    return Keys;
}

void USkillsSubsystem::CargarSkillsCompradas(const TArray<FName>& Compradas)
{
    for (const FName& Id : Compradas)
    {
        if (!NivelesActuales.Contains(Id)) NivelesActuales.Add(Id, 1);
    }
}

TMap<FName, int32> USkillsSubsystem::ObtenerNivelesSkills() const
{
    return NivelesActuales;
}

void USkillsSubsystem::CargarNivelesSkills(const TMap<FName, int32>& Niveles)
{
    NivelesActuales = Niveles;
    if (EstaSkillComprada(FName("DestruirPolvoAreas"))) bAspiradoPolvoActivo = true;
}

FText USkillsSubsystem::ObtenerTextoPrecioSkill(FName IdSkill) const
{
    int32 Costo = ObtenerCostoSkill(IdSkill);

    if (Costo == -1)
    {
        return FText::FromString(TEXT("Max"));
    }

    return FText::AsNumber(Costo);
}

bool USkillsSubsystem::EsNebulosaActivaComprada() const
{
    return EstaSkillComprada(FName("NebulosaActiva"));
}

float USkillsSubsystem::ObtenerPorcentajePolvoNebulosa() const
{
    return (float)ObtenerBonusTotal(ETipoEfectoSkill::NebulosaActiva);
}

float USkillsSubsystem::ObtenerPorcentajePolvoFinoNebulosa() const
{
    // Usamos la CantidadPolvo de la DataTable para el bonus de polvo fino por nebulosa
    return (float)ObtenerCantidadSkill(FName("NebulosaActiva"));
}

bool USkillsSubsystem::EsNebulosaSuperGalacticaActiva() const
{
    return EstaSkillComprada(FName("NebulosaSuperGalactica"));
}

float USkillsSubsystem::ObtenerProbabilidadEvolucionNebulosa() const
{
    return (float)ObtenerBonusTotal(ETipoEfectoSkill::NebulosaSuperGalactica);
}

float USkillsSubsystem::ObtenerRadioMaximoAspirado() const
{
    // Usamos la función ObtenerCantidadSkill que ya existe en esta clase
    int32 RadioConfigurado = ObtenerCantidadSkill(FName("DestruirPolvoAreas"));

    // Si el diseñador configuró un valor mayor a 0 en la DataTable, lo usamos
    if (RadioConfigurado > 0)
    {
        return (float)RadioConfigurado;
    }

    // Si no, devolvemos el RadioMaximo por defecto (la variable que ya tienes declarada)
    return RadioMaximo;
}