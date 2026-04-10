#pragma once

// ============================================================
// i18n helper: g_language 0=EN, 1=JP
// ============================================================
extern int g_language;

inline const char* L(const char* en, const char* jp) {
  return g_language == 1 ? jp : en;
}
