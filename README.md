# HydroSense - A Self-Sustaining Smart Watering System

[中文版 (Chinese Version)](README_zh.md)

## Project Introduction

HydroSense is a smart plant care terminal designed to be **energy self-sufficient**, achieving "perpetual endurance."

The design philosophy combines **ultra-low power consumption** with **efficient energy harvesting**. Every non-essential peripheral is fully powered off when idle, and the e-ink screen refreshes only when readings change. Meanwhile, a hot-swappable solar charging module silently replenishes energy, creating a perfect energy loop, making it ideal for long-term, independent operation in outdoor or off-grid scenarios.

## Core Technologies

- **Full-Link Physical Power Gating**: All non-essential peripherals (like the 12V boost converter, sensors, and e-ink display) are physically power-controlled by independent MOSFET switches. When not in use, their power is completely cut off, achieving true zero-leakage and zero-standby power consumption.

- **Hardware-Level Boot Safety**: By equipping the power switches with strong pull-up resistors, all peripherals are ensured to be in a definite "off" state during the brief "window" of the main chip's power-on initialization, preventing accidental startups and potential hardware conflicts.

- **Modular Solar Recharging**: The solar charging system is designed as an optional, hot-swappable external module. This allows the device to be used indoors as a pure ultra-low-power device or connected to a solar panel outdoors to be upgraded into a fully energy-self-sufficient system.

## Workflow

HydroSense's operation is flexible and intelligent, designed to adapt to different scenarios:

1.  **Autonomous Operation**: In the default mode, the system checks soil moisture every few seconds, waters the plant when the soil is too dry, and **updates the screen snapshot** only when readings change. Switching the device off cuts power to all peripherals and puts the main processor into deep sleep.

2.  **Human-Computer Interaction**: Users can switch to a feature-rich interactive mode at any time. In this mode, the device **activates a real-time interactive interface**, providing an intuitive menu. Users can view historical data charts, manually control watering, and fine-tune various system parameters.

3.  **Intelligent Analysis**: In interactive mode, the system can connect to the cloud and use Large Language Models (LLMs) to perform intelligent analysis on the collected data, providing users with professional plant care advice.

## Planned

- **"Sentinel-Commander" Dual-Core Architecture**: Let the ultra-low-power coprocessor (ULP, Sentinel) take over periodic soil monitoring while the main processor (Commander) stays in deep sleep, and wake the main processor only when watering is needed, to cut average power further.
