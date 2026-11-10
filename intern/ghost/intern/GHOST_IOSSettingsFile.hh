/* SPDX-FileCopyrightText: 2026 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup GHOST
 * The iPad touch layer's own settings as one versioned JSON file, and user customizations of
 * Pencil palette slots and command bar keys.
 *
 * Only this app's touch settings live here; Blender's preferences stay in its own files. The
 * file carries a version and a save time: a newer schema is read field by field, and when two
 * copies disagree the most recently saved one wins.
 */

#pragma once

#include "GHOST_IOSPencilSettings.hh"
#include "GHOST_IOSTouchKeys.hh"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

constexpr int GHOST_IOS_SETTINGS_VERSION = 1;
constexpr int GHOST_IOS_CUSTOM_MAX = 64;
constexpr int GHOST_IOS_CUSTOM_NAME_MAX = 32;
constexpr int GHOST_IOS_CUSTOM_ACTION_MAX = 96;

/** A replaced palette slot or bar key, such as `palette.edit.3` → `mesh.subdivide`. */
struct GHOST_IOSCustomization {
  char name[GHOST_IOS_CUSTOM_NAME_MAX] = "";
  char action[GHOST_IOS_CUSTOM_ACTION_MAX] = "";
};

struct GHOST_IOSSettingsFile {
  int version = GHOST_IOS_SETTINGS_VERSION;
  /** Seconds since 1970 when this copy was saved. */
  double saved_at = 0.0;
  GHOST_IOSTouchSettings touch;
  GHOST_IOSCustomization custom[GHOST_IOS_CUSTOM_MAX];
  int custom_num = 0;

  const char *customAction(const char *name) const
  {
    for (int i = 0; i < custom_num; i++) {
      if (std::strcmp(custom[i].name, name) == 0) {
        return custom[i].action;
      }
    }
    return nullptr;
  }

  /** Replace \a name with \a action, or restore its default when \a action is empty. */
  bool setCustomAction(const char *name, const char *action)
  {
    if (std::strlen(name) >= size_t(GHOST_IOS_CUSTOM_NAME_MAX) ||
        std::strlen(action) >= size_t(GHOST_IOS_CUSTOM_ACTION_MAX))
    {
      return false;
    }
    for (int i = 0; i < custom_num; i++) {
      if (std::strcmp(custom[i].name, name) == 0) {
        if (action[0] == '\0') {
          custom[i] = custom[--custom_num];
          custom[custom_num] = GHOST_IOSCustomization();
        }
        else {
          std::strcpy(custom[i].action, action);
        }
        return true;
      }
    }
    if (action[0] == '\0') {
      return true;
    }
    if (custom_num == GHOST_IOS_CUSTOM_MAX) {
      return false;
    }
    std::strcpy(custom[custom_num].name, name);
    std::strcpy(custom[custom_num].action, action);
    custom_num++;
    return true;
  }
};

/* -------------------------------------------------------------------- */
/** \name JSON
 * \{ */

inline void GHOST_IOSJSON_appendString(std::string &out, const char *text)
{
  out += '"';
  for (const char *c = text; *c; c++) {
    switch (*c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      default:
        if (static_cast<unsigned char>(*c) < 0x20) {
          char escaped[8];
          std::snprintf(escaped, sizeof(escaped), "\\u%04x", *c);
          out += escaped;
        }
        else {
          out += *c;
        }
    }
  }
  out += '"';
}

inline std::string GHOST_IOSSettingsFile_write(const GHOST_IOSSettingsFile &file)
{
  const GHOST_IOSTouchSettings &touch = file.touch;
  char number[64];
  std::string out = "{\n";
  std::snprintf(number, sizeof(number), "  \"version\": %d,\n", file.version);
  out += number;
  std::snprintf(number, sizeof(number), "  \"saved_at\": %.3f,\n", file.saved_at);
  out += number;
  std::snprintf(number, sizeof(number), "  \"double_tap\": %d,\n", int(touch.double_tap));
  out += number;
  std::snprintf(number, sizeof(number), "  \"squeeze\": %d,\n", int(touch.squeeze));
  out += number;
  out += std::string("  \"pencil_only_drawing\": ") +
         (touch.pencil_only_drawing ? "true" : "false") + ",\n";
  out += std::string("  \"touch_spacing\": ") + (touch.touch_spacing ? "true" : "false") + ",\n";
  out += std::string("  \"timeline_minimized\": ") + (touch.timeline_minimized ? "true" : "false") +
         ",\n";
  out += "  \"tap_gestures\": [";
  for (int i = 0; i < GHOST_IOS_TAP_GESTURE_COUNT; i++) {
    std::snprintf(number, sizeof(number), "%s%d", i ? ", " : "", int(touch.tap_gestures[i]));
    out += number;
  }
  out += "],\n  \"custom\": {";
  for (int i = 0; i < file.custom_num; i++) {
    out += i ? ",\n    " : "\n    ";
    GHOST_IOSJSON_appendString(out, file.custom[i].name);
    out += ": ";
    GHOST_IOSJSON_appendString(out, file.custom[i].action);
  }
  out += file.custom_num ? "\n  }\n}\n" : "}\n}\n";
  return out;
}

/** A small reader for the settings file. Unknown keys and value types are skipped. */
class GHOST_IOSJSONReader {
 public:
  explicit GHOST_IOSJSONReader(const char *text) : p_(text) {}

  bool readFile(GHOST_IOSSettingsFile *file)
  {
    if (!expect('{')) {
      return false;
    }
    if (peek() == '}') {
      p_++;
      return true;
    }
    do {
      std::string key;
      if (!readString(key) || !expect(':')) {
        return false;
      }
      if (!readField(key, file)) {
        return false;
      }
    } while (consume(','));
    return expect('}');
  }

 private:
  bool readField(const std::string &key, GHOST_IOSSettingsFile *file)
  {
    GHOST_IOSTouchSettings &touch = file->touch;
    double value = 0.0;
    bool flag = false;
    if (key == "version") {
      if (!readNumber(&value)) {
        return false;
      }
      file->version = int(value);
    }
    else if (key == "saved_at") {
      if (!readNumber(&file->saved_at)) {
        return false;
      }
    }
    else if (key == "double_tap") {
      if (!readNumber(&value)) {
        return false;
      }
      touch.double_tap = GHOST_IOSDoubleTapChoice_fromStored(long(value));
    }
    else if (key == "squeeze") {
      if (!readNumber(&value)) {
        return false;
      }
      touch.squeeze = GHOST_IOSSqueezeChoice_fromStored(long(value));
    }
    else if (key == "pencil_only_drawing") {
      if (!readBool(&flag)) {
        return false;
      }
      touch.pencil_only_drawing = flag;
    }
    else if (key == "touch_spacing") {
      if (!readBool(&flag)) {
        return false;
      }
      touch.touch_spacing = flag;
    }
    else if (key == "timeline_minimized") {
      if (!readBool(&flag)) {
        return false;
      }
      touch.timeline_minimized = flag;
    }
    else if (key == "tap_gestures") {
      if (!expect('[')) {
        return false;
      }
      int index = 0;
      if (peek() != ']') {
        do {
          if (!readNumber(&value)) {
            return false;
          }
          if (index < GHOST_IOS_TAP_GESTURE_COUNT) {
            touch.tap_gestures[index] = GHOST_IOSGestureAction_fromStored(
                long(value), touch.tap_gestures[index]);
          }
          index++;
        } while (consume(','));
      }
      if (!expect(']')) {
        return false;
      }
    }
    else if (key == "custom") {
      if (!expect('{')) {
        return false;
      }
      file->custom_num = 0;
      if (peek() != '}') {
        do {
          std::string name, action;
          if (!readString(name) || !expect(':') || !readString(action)) {
            return false;
          }
          file->setCustomAction(name.c_str(), action.c_str());
        } while (consume(','));
      }
      if (!expect('}')) {
        return false;
      }
    }
    else {
      return skipValue();
    }
    return true;
  }

  void skipSpace()
  {
    while (*p_ == ' ' || *p_ == '\n' || *p_ == '\r' || *p_ == '\t') {
      p_++;
    }
  }
  char peek()
  {
    skipSpace();
    return *p_;
  }
  bool consume(const char c)
  {
    if (peek() == c) {
      p_++;
      return true;
    }
    return false;
  }
  bool expect(const char c)
  {
    return consume(c);
  }
  bool readString(std::string &out)
  {
    if (!consume('"')) {
      return false;
    }
    out.clear();
    while (*p_ && *p_ != '"') {
      if (*p_ == '\\') {
        p_++;
        switch (*p_) {
          case 'n':
            out += '\n';
            break;
          case 't':
            out += '\t';
            break;
          case 'u': {
            /* Only ASCII escapes are written; keep anything else as '?'. */
            char hex[5] = {0};
            for (int i = 0; i < 4 && p_[1]; i++) {
              hex[i] = *++p_;
            }
            const long code = std::strtol(hex, nullptr, 16);
            out += code > 0 && code < 0x80 ? char(code) : '?';
            break;
          }
          case '\0':
            return false;
          default:
            out += *p_;
        }
        p_++;
        continue;
      }
      out += *p_++;
    }
    return consume('"');
  }
  bool readNumber(double *r_value)
  {
    skipSpace();
    char *end = nullptr;
    *r_value = std::strtod(p_, &end);
    if (end == p_) {
      return false;
    }
    p_ = end;
    return true;
  }
  bool readBool(bool *r_value)
  {
    skipSpace();
    if (std::strncmp(p_, "true", 4) == 0) {
      p_ += 4;
      *r_value = true;
      return true;
    }
    if (std::strncmp(p_, "false", 5) == 0) {
      p_ += 5;
      *r_value = false;
      return true;
    }
    return false;
  }
  bool skipValue()
  {
    const char c = peek();
    if (c == '"') {
      std::string ignored;
      return readString(ignored);
    }
    if (c == '{' || c == '[') {
      const char close = c == '{' ? '}' : ']';
      p_++;
      if (consume(close)) {
        return true;
      }
      do {
        if (c == '{') {
          std::string ignored;
          if (!readString(ignored) || !expect(':')) {
            return false;
          }
        }
        if (!skipValue()) {
          return false;
        }
      } while (consume(','));
      return expect(close);
    }
    if (c == 't' || c == 'f') {
      bool ignored;
      return readBool(&ignored);
    }
    if (std::strncmp(p_, "null", 4) == 0) {
      p_ += 4;
      return true;
    }
    double ignored;
    return readNumber(&ignored);
  }

  const char *p_;
};

/** Read \a json into \a r_file, which keeps its defaults for missing fields. */
inline bool GHOST_IOSSettingsFile_read(const char *json, GHOST_IOSSettingsFile *r_file)
{
  GHOST_IOSSettingsFile file;
  GHOST_IOSJSONReader reader(json);
  if (!reader.readFile(&file)) {
    return false;
  }
  *r_file = file;
  return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Custom actions
 * \{ */

enum class GHOST_IOSCustomKind : uint8_t {
  Invalid,
  /** A key with modifiers, such as `Shift+D`. */
  Key,
  /** An operator call, such as `mesh.subdivide` or `wm.call_menu name=VIEW3D_MT_add`. */
  Command,
};

struct GHOST_IOSCustomAction {
  GHOST_IOSCustomKind kind = GHOST_IOSCustomKind::Invalid;
  GHOST_IOSKeyStroke stroke = {GHOST_kKeyUnknown, 0, nullptr};
  char command[GHOST_IOS_COMMAND_MAX] = "";
};

inline bool GHOST_IOS_equalsNoCase(const char *a, const char *b)
{
  for (; *a && *b; a++, b++) {
    if (std::tolower(static_cast<unsigned char>(*a)) !=
        std::tolower(static_cast<unsigned char>(*b)))
    {
      return false;
    }
  }
  return *a == *b;
}

/** Text typed by a lone letter or digit key. */
inline const char *GHOST_IOS_keyText(const char c)
{
  static const char *const letters =
      "a\0b\0c\0d\0e\0f\0g\0h\0i\0j\0k\0l\0m\0n\0o\0p\0q\0r\0s\0t\0u\0v\0w"
      "\0x\0y\0z\0";
  static const char *const digits = "0\0001\0002\0003\0004\0005\0006\0007\0008\0009\0";
  if (c >= 'a' && c <= 'z') {
    return letters + (c - 'a') * 2;
  }
  if (c >= '0' && c <= '9') {
    return digits + (c - '0') * 2;
  }
  return nullptr;
}

inline bool GHOST_IOS_parseKeyName(const char *name, GHOST_IOSKeyStroke *stroke)
{
  const size_t len = std::strlen(name);
  if (len == 1 && std::isalnum(static_cast<unsigned char>(name[0]))) {
    const char c = char(std::tolower(static_cast<unsigned char>(name[0])));
    stroke->key = c >= 'a' ? GHOST_TKey(int(GHOST_kKeyA) + (c - 'a')) :
                             GHOST_TKey(int(GHOST_kKey0) + (c - '0'));
    stroke->utf8 = GHOST_IOS_keyText(c);
    return true;
  }
  if ((name[0] == 'F' || name[0] == 'f') && len >= 2 && len <= 3) {
    const int number = std::atoi(name + 1);
    if (number >= 1 && number <= 12) {
      stroke->key = GHOST_TKey(int(GHOST_kKeyF1) + number - 1);
      return true;
    }
  }
  if (std::strncmp(name, "Numpad", 6) == 0 && len == 7 && std::isdigit(name[6])) {
    stroke->key = GHOST_TKey(int(GHOST_kKeyNumpad0) + (name[6] - '0'));
    return true;
  }
  struct Named {
    const char *name;
    GHOST_TKey key;
  };
  static const Named named[] = {
      {"Tab", GHOST_kKeyTab},
      {"Esc", GHOST_kKeyEsc},
      {"Escape", GHOST_kKeyEsc},
      {"Return", GHOST_kKeyEnter},
      {"Enter", GHOST_kKeyEnter},
      {"Delete", GHOST_kKeyDelete},
      {"Del", GHOST_kKeyDelete},
      {"Backspace", GHOST_kKeyBackSpace},
      {"Space", GHOST_kKeySpace},
      {"Minus", GHOST_kKeyMinus},
      {"Period", GHOST_kKeyPeriod},
      {"Home", GHOST_kKeyHome},
      {"End", GHOST_kKeyEnd},
      {"Up", GHOST_kKeyUpArrow},
      {"Down", GHOST_kKeyDownArrow},
      {"Left", GHOST_kKeyLeftArrow},
      {"Right", GHOST_kKeyRightArrow},
  };
  for (const Named &entry : named) {
    if (GHOST_IOS_equalsNoCase(name, entry.name)) {
      stroke->key = entry.key;
      return true;
    }
  }
  return false;
}

/** Whether \a text looks like an operator id: `lowercase.lowercase` with digits and underscores.
 */
inline bool GHOST_IOS_isOperatorId(const char *text, const size_t len)
{
  int dots = 0;
  for (size_t i = 0; i < len; i++) {
    const char c = text[i];
    if (c == '.') {
      if (i == 0 || i + 1 == len) {
        return false;
      }
      dots++;
    }
    else if (!(std::islower(static_cast<unsigned char>(c)) ||
               std::isdigit(static_cast<unsigned char>(c)) || c == '_'))
    {
      return false;
    }
  }
  return dots == 1;
}

/**
 * Parse what the user typed for a slot or key:
 * - `Shift+D`, `Ctrl+Shift+Z`, `Tab`, `F3`: a key.
 * - `mesh.subdivide`, `object.shade_smooth keep_sharp_edges=1`: an operator with up to one
 * property.
 * - `VIEW3D_MT_add`, `VIEW3D_MT_pivot_pie`: a menu or pie menu.
 */
inline bool GHOST_IOSCustomAction_parse(const char *text, GHOST_IOSCustomAction *r_action)
{
  GHOST_IOSCustomAction action;
  while (*text == ' ') {
    text++;
  }
  size_t len = std::strlen(text);
  while (len > 0 && text[len - 1] == ' ') {
    len--;
  }
  if (len == 0 || len >= size_t(GHOST_IOS_CUSTOM_ACTION_MAX)) {
    return false;
  }
  std::string trimmed(text, len);

  /* Menus. */
  if (trimmed.find("_MT_") != std::string::npos && trimmed.find(' ') == std::string::npos) {
    const bool pie = GHOST_IOS_hasSuffix(trimmed.c_str(), "_pie");
    action.kind = GHOST_IOSCustomKind::Command;
    if (!GHOST_IOSCommand_format(
            action.command, pie ? "wm.call_menu_pie" : "wm.call_menu", "name", trimmed.c_str()))
    {
      return false;
    }
    *r_action = action;
    return true;
  }

  /* Operators, with an optional `name=value`. */
  const size_t space = trimmed.find(' ');
  const std::string idname = trimmed.substr(0, space);
  if (GHOST_IOS_isOperatorId(idname.c_str(), idname.size())) {
    action.kind = GHOST_IOSCustomKind::Command;
    if (space == std::string::npos) {
      if (!GHOST_IOSCommand_format(action.command, idname.c_str())) {
        return false;
      }
    }
    else {
      const std::string property = trimmed.substr(space + 1);
      const size_t equals = property.find('=');
      if (equals == std::string::npos || equals == 0 || property.find(' ') != std::string::npos) {
        return false;
      }
      if (!GHOST_IOSCommand_format(action.command,
                                   idname.c_str(),
                                   property.substr(0, equals).c_str(),
                                   property.substr(equals + 1).c_str()))
      {
        return false;
      }
    }
    *r_action = action;
    return true;
  }

  /* Keys: modifiers joined with `+`, then the key. */
  uint8_t modifiers = 0;
  size_t start = 0;
  while (true) {
    const size_t plus = trimmed.find('+', start);
    const std::string part = trimmed.substr(
        start, plus == std::string::npos ? std::string::npos : plus - start);
    if (plus == std::string::npos) {
      if (!GHOST_IOS_parseKeyName(part.c_str(), &action.stroke)) {
        return false;
      }
      break;
    }
    const char *name = part.c_str();
    if (GHOST_IOS_equalsNoCase(name, "Shift")) {
      modifiers |= GHOST_IOS_STROKE_SHIFT;
    }
    else if (GHOST_IOS_equalsNoCase(name, "Ctrl") || GHOST_IOS_equalsNoCase(name, "Control")) {
      modifiers |= GHOST_IOS_STROKE_CTRL;
    }
    else if (GHOST_IOS_equalsNoCase(name, "Alt") || GHOST_IOS_equalsNoCase(name, "Option")) {
      modifiers |= GHOST_IOS_STROKE_ALT;
    }
    else if (GHOST_IOS_equalsNoCase(name, "Cmd") || GHOST_IOS_equalsNoCase(name, "Command")) {
      modifiers |= GHOST_IOS_STROKE_OS;
    }
    else {
      return false;
    }
    start = plus + 1;
  }
  action.kind = GHOST_IOSCustomKind::Key;
  action.stroke.modifiers = modifiers;
  *r_action = action;
  return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Customized key sets
 * \{ */

inline const char *GHOST_IOSPalette_setName(const GHOST_IOSInteractionContext &context)
{
  switch (context.mode) {
    case GHOST_IOSMode::Edit:
      return "edit";
    case GHOST_IOSMode::Sculpt:
      return "sculpt";
    case GHOST_IOSMode::Paint:
      return "paint";
    case GHOST_IOSMode::Object:
    case GHOST_IOSMode::Other:
      break;
  }
  return "object";
}

inline const char *GHOST_IOSCommandBar_setName(const GHOST_IOSInteractionContext &context)
{
  if (context.editor == GHOST_IOSEditor::Node) {
    return "node";
  }
  if (context.editor == GHOST_IOSEditor::View3D || context.editor == GHOST_IOSEditor::Image) {
    switch (context.mode) {
      case GHOST_IOSMode::Object:
        return "object";
      case GHOST_IOSMode::Edit:
        return "edit";
      case GHOST_IOSMode::Sculpt:
        return "sculpt";
      case GHOST_IOSMode::Paint:
        return "paint";
      case GHOST_IOSMode::Other:
        break;
    }
  }
  return "other";
}

/** `palette.<set>.<index>` or `bar.<set>.<index>`. */
inline void GHOST_IOSCustom_name(char name[GHOST_IOS_CUSTOM_NAME_MAX],
                                 const char *group,
                                 const char *set,
                                 const int index)
{
  std::snprintf(name, GHOST_IOS_CUSTOM_NAME_MAX, "%s.%s.%d", group, set, index);
}

/** Show a custom action on a slot or key: the text itself, shortened to fit. */
inline void GHOST_IOSCustom_label(char label[24], const char *action)
{
  const char *dot = std::strrchr(action, '.');
  const char *shown = dot != nullptr && std::strchr(action, ' ') == nullptr ? dot + 1 : action;
  std::snprintf(label, 24, "%s", shown);
}

/** The palette for \a context with the user's replacements applied. */
struct GHOST_IOSCustomPalette {
  GHOST_IOSPaletteSlot slots[GHOST_IOS_PALETTE_SLOT_COUNT];
  char labels[GHOST_IOS_PALETTE_SLOT_COUNT][24];
  char commands[GHOST_IOS_PALETTE_SLOT_COUNT][GHOST_IOS_COMMAND_MAX];
  char set[16];

  void build(const GHOST_IOSInteractionContext &context, const GHOST_IOSSettingsFile &file)
  {
    std::snprintf(set, sizeof(set), "%s", GHOST_IOSPalette_setName(context));
    const GHOST_IOSPaletteSlot *defaults = GHOST_IOSPalette_slots(context);
    for (int i = 0; i < GHOST_IOS_PALETTE_SLOT_COUNT; i++) {
      slots[i] = defaults[i];
      char name[GHOST_IOS_CUSTOM_NAME_MAX];
      GHOST_IOSCustom_name(name, "palette", set, i);
      const char *text = file.customAction(name);
      GHOST_IOSCustomAction action;
      if (text == nullptr || !GHOST_IOSCustomAction_parse(text, &action)) {
        continue;
      }
      GHOST_IOSCustom_label(labels[i], text);
      slots[i].title = labels[i];
      slots[i].symbol = "star";
      if (action.kind == GHOST_IOSCustomKind::Key) {
        slots[i].kind = GHOST_IOSSlotKind::Key;
        slots[i].stroke = action.stroke;
        slots[i].command = nullptr;
      }
      else {
        std::strcpy(commands[i], action.command);
        slots[i].kind = GHOST_IOSSlotKind::Command;
        slots[i].stroke = {GHOST_kKeyUnknown, 0, nullptr};
        slots[i].command = commands[i];
      }
    }
  }
};

/** The command bar's context keys for \a context with the user's replacements applied. */
struct GHOST_IOSCustomBar {
  GHOST_IOSBarKey keys[GHOST_IOS_BAR_KEY_MAX];
  char labels[GHOST_IOS_BAR_KEY_MAX][24];
  char commands[GHOST_IOS_BAR_KEY_MAX][GHOST_IOS_COMMAND_MAX];
  char set[16];
  int count = 0;

  void build(const GHOST_IOSInteractionContext &context, const GHOST_IOSSettingsFile &file)
  {
    /* A running tool's own shortcuts take the place of the mode's keys until it finishes. */
    if (context.modal_keys_num > 0) {
      std::snprintf(set, sizeof(set), "%s", "modal");
      count = std::min(context.modal_keys_num, GHOST_IOS_BAR_KEY_MAX);
      for (int i = 0; i < count; i++) {
        const GHOST_IOSModalKey &modal = context.modal_keys[i];
        std::snprintf(labels[i], sizeof(labels[i]), "%s", modal.title);
        keys[i].title = labels[i];
        keys[i].stroke = {GHOST_TKey(modal.key), modal.modifiers, nullptr};
        keys[i].command = nullptr;
      }
      return;
    }
    std::snprintf(set, sizeof(set), "%s", GHOST_IOSCommandBar_setName(context));
    const GHOST_IOSBarKey *defaults = GHOST_IOSCommandBar_keys(context, &count);
    for (int i = 0; i < count; i++) {
      keys[i] = defaults[i];
      char name[GHOST_IOS_CUSTOM_NAME_MAX];
      GHOST_IOSCustom_name(name, "bar", set, i);
      const char *text = file.customAction(name);
      GHOST_IOSCustomAction action;
      if (text == nullptr || !GHOST_IOSCustomAction_parse(text, &action)) {
        continue;
      }
      GHOST_IOSCustom_label(labels[i], text);
      keys[i].title = labels[i];
      if (action.kind == GHOST_IOSCustomKind::Key) {
        keys[i].stroke = action.stroke;
        keys[i].command = nullptr;
      }
      else {
        std::strcpy(commands[i], action.command);
        keys[i].stroke = {GHOST_kKeyUnknown, 0, nullptr};
        keys[i].command = commands[i];
      }
    }
  }
};

/** \} */
