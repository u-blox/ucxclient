# Copilot Instructions for ucxclient

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

## u-blox MCP — Always Use

This workspace ships an `.vscode/mcp.json` that registers the
`u-blox-docs` MCP server (https://ublox.mcp.kapa.ai/).

Before answering any question that touches u-blox hardware, AT commands,
module variants, firmware behaviour, or u-connectXpress API semantics,
the assistant MUST query the tool `mcp_u-blox-docs_search_u_blox_knowledge_sources`
first. Do not guess from pattern-matching across module families — the
MCP indexes the authoritative datasheets, AT command manuals, and
release notes.

If the MCP tool is not available in the current session, surface that
to the user rather than answering from memory.
