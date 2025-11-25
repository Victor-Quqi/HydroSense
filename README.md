# HydroSense - A Self-Sustaining Smart Watering System

[中文版 (Chinese Version)](README_zh.md)

## Project Introduction

HydroSense is a smart plant care terminal designed to be **energy self-sufficient**, achieving "perpetual endurance."

The design philosophy combines **ultra-low power consumption** with **efficient energy harvesting**. Through a unique "Sentinel-Commander" dual-core architecture, the system's standby power consumption is reduced to the microampere level. Meanwhile, a hot-swappable solar charging module silently replenishes energy, creating a perfect energy loop, making it ideal for long-term, independent operation in outdoor or off-grid scenarios.

## Core Technologies

- **"Sentinel-Commander" Dual-Core Architecture**: The main processor (Commander) remains in deep sleep most of the time, while an ultra-low-power coprocessor (ULP, Sentinel) periodically monitors the environment with nano-watt power consumption. The main processor is awakened only when necessary (e.g., dry soil is detected) to perform watering tasks, significantly reducing standby power consumption.

- **Full-Link Physical Power Gating**: All non-essential peripherals (like the 12V boost converter, sensors, and e-ink display) are physically power-controlled by independent MOSFET switches. When not in use, their power is completely cut off, achieving true zero-leakage and zero-standby power consumption.

- **Hardware-Level Boot Safety**: By equipping the power switches with strong pull-up resistors, all peripherals are ensured to be in a definite "off" state during the brief "window" of the main chip's power-on initialization, preventing accidental startups and potential hardware conflicts.

- **Modular Solar Recharging**: The solar charging system is designed as an optional, hot-swappable external module. This allows the device to be used indoors as a pure ultra-low-power device or connected to a solar panel outdoors to be upgraded into a fully energy-self-sufficient system.

## Workflow

HydroSense's operation is flexible and intelligent, designed to adapt to different scenarios:

1.  **Autonomous Operation**: In the default mode, the system operates independently with extreme energy efficiency. It periodically monitors soil moisture using the ultra-low-power "Sentinel" core and only wakes up the "Commander" main processor to perform watering tasks and **update the screen snapshot** when needed. After the task is completed, it automatically returns to deep sleep.

2.  **Human-Computer Interaction**: Users can switch to a feature-rich interactive mode at any time. In this mode, the device **activates a real-time interactive interface**, providing an intuitive menu. Users can view historical data charts, manually control watering, and fine-tune various system parameters.

3.  **Intelligent Analysis**: In interactive mode, the system can connect to the cloud and use Large Language Models (LLMs) to perform intelligent analysis on the collected data, providing users with professional plant care advice.