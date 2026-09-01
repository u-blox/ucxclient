# Copilot Instructions for u-connectClient

## Critical Rules

**NEVER run `git push` or any command that modifies remote repositories.**

- Do NOT push commits
- Do NOT force push
- Do NOT delete remote branches
- Do NOT create pull requests automatically

Always let the user review and push changes manually.

## Project Context

This is an embedded C project for u-blox u-connectXpress modules with:
- STM32F4 examples using FreeRTOS
- Renode emulator support for testing
- PyInvoke build automation
