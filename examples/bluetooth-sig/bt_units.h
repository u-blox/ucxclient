/*
 * Copyright 2025 u-blox
 *
 * Auto-generated from Bluetooth SIG units.yaml
 * Do not edit manually - regenerate using convert_yaml_to_c.py
 */

#ifndef BT_UNITS_H
#define BT_UNITS_H

#include <stdint.h>

typedef struct {
    uint16_t uuid;
    const char *name;
    const char *symbol;  // Common symbol (e.g., "m", "kg", "°C")
} BtUnit_t;

static const BtUnit_t BT_UNITS[] = {
    {0x2700, "unitless", ""},
    {0x2701, "length (metre)", "m"},
    {0x2702, "mass (kilogram)", "kg"},
    {0x2703, "time (second)", "s"},
    {0x2704, "electric current (ampere)", "A"},
    {0x2705, "thermodynamic temperature (kelvin)", "K"},
    {0x2706, "amount of substance (mole)", ""},
    {0x2707, "luminous intensity (candela)", ""},
    {0x2710, "area (square metres)", "m"},
    {0x2711, "volume (cubic metres)", "m"},
    {0x2712, "velocity (metres per second)", "m"},
    {0x2713, "acceleration (metres per second squared)", "m"},
    {0x2714, "wavenumber (reciprocal metre)", "m"},
    {0x2715, "density (kilogram per cubic metre)", "m"},
    {0x2716, "surface density (kilogram per square metre)", "m"},
    {0x2717, "specific volume (cubic metre per kilogram)", "m"},
    {0x2718, "current density (ampere per square metre)", "m"},
    {0x2719, "magnetic field strength (ampere per metre)", "m"},
    {0x271A, "amount concentration (mole per cubic metre)", "m"},
    {0x271B, "mass concentration (kilogram per cubic metre)", "m"},
    {0x271C, "luminance (candela per square metre)", "m"},
    {0x271D, "refractive index", ""},
    {0x271E, "relative permeability", ""},
    {0x2720, "plane angle (radian)", "rad"},
    {0x2721, "solid angle (steradian)", "rad"},
    {0x2722, "frequency (hertz)", "Hz"},
    {0x2723, "force (newton)", ""},
    {0x2724, "pressure (pascal)", "Pa"},
    {0x2725, "energy (joule)", "J"},
    {0x2726, "power (watt)", "W"},
    {0x2727, "electric charge (coulomb)", "C"},
    {0x2728, "electric potential difference (volt)", "V"},
    {0x2729, "capacitance (farad)", "F"},
    {0x272A, "electric resistance (ohm)", "Ω"},
    {0x272B, "electric conductance (siemens)", ""},
    {0x272C, "magnetic flux (weber)", "lx"},
    {0x272D, "magnetic flux density (tesla)", "lx"},
    {0x272E, "inductance (henry)", "H"},
    {0x272F, "Celsius temperature (degree Celsius)", "°C"},
    {0x2730, "luminous flux (lumen)", "lm"},
    {0x2731, "illuminance (lux)", "lx"},
    {0x2732, "activity referred to a radionuclide (becquerel)", ""},
    {0x2733, "absorbed dose (gray)", ""},
    {0x2734, "dose equivalent (sievert)", ""},
    {0x2735, "catalytic activity (katal)", ""},
    {0x2740, "dynamic viscosity (pascal second)", "s"},
    {0x2741, "moment of force (newton metre)", "m"},
    {0x2742, "surface tension (newton per metre)", "m"},
    {0x2743, "angular velocity (radian per second)", "s"},
    {0x2744, "angular acceleration (radian per second squared)", "s"},
    {0x2745, "heat flux density (watt per square metre)", "m"},
    {0x2746, "heat capacity (joule per kelvin)", "K"},
    {0x2747, "specific heat capacity (joule per kilogram kelvin)", "kg"},
    {0x2748, "specific energy (joule per kilogram)", "kg"},
    {0x2749, "thermal conductivity (watt per metre kelvin)", "m"},
    {0x274A, "energy density (joule per cubic metre)", "m"},
    {0x274B, "electric field strength (volt per metre)", "m"},
    {0x274C, "electric charge density (coulomb per cubic metre)", "m"},
    {0x274D, "surface charge density (coulomb per square metre)", "m"},
    {0x274E, "electric flux density (coulomb per square metre)", "m"},
    {0x274F, "permittivity (farad per metre)", "m"},
    {0x2750, "permeability (henry per metre)", "m"},
    {0x2751, "molar energy (joule per mole)", "J"},
    {0x2752, "molar entropy (joule per mole kelvin)", "K"},
    {0x2753, "exposure (coulomb per kilogram)", "kg"},
    {0x2754, "absorbed dose rate (gray per second)", "s"},
    {0x2755, "radiant intensity (watt per steradian)", "W"},
    {0x2756, "radiance (watt per square metre steradian)", "m"},
    {0x2757, "catalytic activity concentration (katal per cubic metre)", "m"},
    {0x2760, "time (minute)", ""},
    {0x2761, "time (hour)", ""},
    {0x2762, "time (day)", ""},
    {0x2763, "plane angle (degree)", "°"},
    {0x2764, "plane angle (minute)", ""},
    {0x2765, "plane angle (second)", "s"},
    {0x2766, "area (hectare)", ""},
    {0x2767, "volume (litre)", "L"},
    {0x2768, "mass (tonne)", ""},
    {0x2780, "pressure (bar)", ""},
    {0x2781, "pressure (millimetre of mercury)", "m"},
    {0x2782, "length (ångström)", ""},
    {0x2783, "length (nautical mile)", ""},
    {0x2784, "area (barn)", ""},
    {0x2785, "velocity (knot)", ""},
    {0x2786, "logarithmic radio quantity (neper)", ""},
    {0x2787, "logarithmic radio quantity (bel)", ""},
    {0x27A0, "length (yard)", ""},
    {0x27A1, "length (parsec)", ""},
    {0x27A2, "length (inch)", ""},
    {0x27A3, "length (foot)", ""},
    {0x27A4, "length (mile)", ""},
    {0x27A5, "pressure (pound-force per square inch)", ""},
    {0x27A6, "velocity (kilometre per hour)", "m"},
    {0x27A7, "velocity (mile per hour)", ""},
    {0x27A8, "angular velocity (revolution per minute)", ""},
    {0x27A9, "energy (gram calorie)", "g"},
    {0x27AA, "energy (kilogram calorie)", "kg"},
    {0x27AB, "energy (kilowatt hour)", "W"},
    {0x27AC, "thermodynamic temperature (degree Fahrenheit)", "°F"},
    {0x27AD, "percentage", "%"},
    {0x27AE, "per mille", ""},
    {0x27AF, "period (beats per minute)", ""},
    {0x27B0, "electric charge (ampere hours)", "A"},
    {0x27B1, "mass density (milligram per decilitre)", "L"},
    {0x27B2, "mass density (millimole per litre)", "L"},
    {0x27B3, "time (year)", ""},
    {0x27B4, "time (month)", ""},
    {0x27B5, "concentration (count per cubic metre)", "m"},
    {0x27B6, "irradiance (watt per square metre)", "m"},
    {0x27B7, "milliliter (per kilogram per minute)", "kg"},
    {0x27B8, "mass (pound)", ""},
    {0x27B9, "metabolic equivalent", ""},
    {0x27BA, "step (per minute)", ""},
    {0x27BC, "stroke (per minute)", ""},
    {0x27BD, "pace (kilometre per minute)", "m"},
    {0x27BE, "luminous efficacy (lumen per watt)", "W"},
    {0x27BF, "luminous energy (lumen hour)", "lm"},
    {0x27C0, "luminous exposure (lux hour)", "lx"},
    {0x27C1, "mass flow (gram per second)", "s"},
    {0x27C2, "volume flow (litre per second)", "s"},
    {0x27C3, "sound pressure (decibel)", ""},
    {0x27C4, "parts per million", ""},
    {0x27C5, "parts per billion", ""},
    {0x27C6, "mass density rate ((milligram per decilitre) per minute)", "L"},
    {0x27C7, "Electrical Apparent Energy (kilovolt ampere hour)", "A"},
    {0x27C8, "Electrical Apparent Power (volt ampere)", "A"},
    {0x27C9, "Gravity (g\textsubscript{n})", ""},
};

#define BT_UNITS_COUNT 127

static inline const char* btGetUnitName(uint16_t uuid) {
    for (int i = 0; i < BT_UNITS_COUNT; i++) {
        if (BT_UNITS[i].uuid == uuid) {
            return BT_UNITS[i].name;
        }
    }
    return NULL;
}

static inline const char* btGetUnitSymbol(uint16_t uuid) {
    for (int i = 0; i < BT_UNITS_COUNT; i++) {
        if (BT_UNITS[i].uuid == uuid) {
            return BT_UNITS[i].symbol;
        }
    }
    return NULL;
}

#endif /* BT_UNITS_H */
