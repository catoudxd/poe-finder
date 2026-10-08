// Poe Finder - listet alle 60 Geisterseelen (Poe Souls) aus Twilight Princess mit Fundort auf
// und zeigt anhand der Spielstand-Flags, welche noch fehlen.
//
// Die Flag-Tabelle stammt aus Dusklights eigener Kartenzaehler-Tabelle (d_menu_map_common.cpp)
// und wurde mit den Poe-Checks des Dusklight-Randomizers (locations.yaml) abgeglichen.

// Standard-Header muessen vor den Spiel-Headern stehen: diese definieren unter MSVC
// "nullptr" als Makro um, was die Standardbibliothek sonst nicht mehr kompilieren laesst.
#include <cstdint>
#include <string>

#include "mods/service.hpp"
#include "mods/svc/log.h"
#include "mods/svc/ui.h"

#include "d/d_com_inf_game.h"
#include "d/d_stage.h"

// Die Spiel-Header definieren NULL/nullptr als Makros um; fuer den eigenen Code zuruecksetzen.
#ifdef nullptr
#undef nullptr
#endif

DEFINE_MOD();
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(UiService, svc_ui);

namespace {

enum Group : uint8_t {
    GROUP_FARON,
    GROUP_ELDIN,
    GROUP_LANAYRU,
    GROUP_SNOWPEAK,
    GROUP_DESERT,
    GROUP_DUNGEON,
    GROUP_COUNT,
};

const char* const kGroupNames[GROUP_COUNT] = {
    "Phirone (Faron)",
    "Eldin",
    "Ranelle (Lanayru)",
    "Schneeberg (Snowpeak)",
    "Gerudo-Wüste",
    "Dungeons",
};

struct Poe {
    Group group;
    const char* where;   // deutsche Ortsbeschreibung
    const char* english; // Name wie im Randomizer / in englischen Guides
    uint8_t saveId;      // Speicherbereich (Region) des Flags
    uint8_t sw;          // Schalter-Flag, das beim Einsammeln gesetzt wird
    bool nightOnly;      // im Freien: erscheint nur nachts
};

// clang-format off
const Poe kPoes[] = {
    // --- Phirone ---
    {GROUP_FARON, "Wald von Phirone: im Nebelgebiet", "Faron Mist Poe", 0x02, 0x5D, true},
    {GROUP_FARON, "Hyrule-Ebene (Phirone)", "Faron Field Poe", 0x06, 0x39, true},
    {GROUP_FARON, "Verlorene Wälder: hinter dem Felsbrocken", "Lost Woods Boulder Poe", 0x07, 0x10, true},
    {GROUP_FARON, "Verlorene Wälder: beim Wasserfall", "Lost Woods Waterfall Poe", 0x07, 0x11, true},
    {GROUP_FARON, "Heiliger Hain: beim Master-Schwert-Podest", "Sacred Grove Master Sword Poe", 0x07, 0x0F, true},
    {GROUP_FARON, "Heiliger Hain: bei der Eulenstatue vor dem Zeitschrein", "Sacred Grove Temple of Time Owl Statue Poe", 0x07, 0x1E, true},

    // --- Eldin ---
    {GROUP_ELDIN, "Hyrule-Ebene (Eldin): Kakariko-Schlucht", "Kakariko Gorge Poe", 0x06, 0x3A, true},
    {GROUP_ELDIN, "Laternenhöhle bei der Kakariko-Schlucht", "Eldin Lantern Cave Poe", 0x19, 0x60, false},
    {GROUP_ELDIN, "Kakariko: beim Bombenladen", "Kakariko Village Bomb Shop Poe", 0x03, 0x5E, true},
    {GROUP_ELDIN, "Kakariko: beim Wachturm", "Kakariko Village Watchtower Poe", 0x03, 0x5F, true},
    {GROUP_ELDIN, "Friedhof von Kakariko: offen auf dem Friedhof", "Kakariko Graveyard Open Poe", 0x03, 0x58, true},
    {GROUP_ELDIN, "Friedhof von Kakariko: hinter/unter einem Grabstein", "Kakariko Graveyard Grave Poe", 0x03, 0x57, true},
    {GROUP_ELDIN, "Todesberg-Pfad", "Death Mountain Trail Poe", 0x03, 0x59, true},
    {GROUP_ELDIN, "Vergessenes Dorf", "Hidden Village Poe", 0x03, 0x40, true},

    // --- Ranelle ---
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): bei der Brücke", "Lanayru Field Bridge Poe", 0x06, 0x33, true},
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): Poe-Grotte, linker Poe", "Lanayru Field Poe Grotto Left Poe", 0x1B, 0x0B, false},
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): Poe-Grotte, rechter Poe", "Lanayru Field Poe Grotto Right Poe", 0x1B, 0x0A, false},
    {GROUP_LANAYRU, "Hyrule-Ebene: Amphitheater-Ruine bei Hyrule-Stadt", "Hyrule Field Amphitheater Poe", 0x06, 0x49, true},
    {GROUP_LANAYRU, "Brücke östlich von Hyrule-Stadt", "East Castle Town Bridge Poe", 0x06, 0x47, true},
    {GROUP_LANAYRU, "Hyrule-Stadt: in Jovanis Haus", "Jovani House Poe", 0x09, 0x1E, false},
    {GROUP_LANAYRU, "Vor dem Südtor von Hyrule-Stadt", "Outside South Castle Town Poe", 0x06, 0x30, true},
    {GROUP_LANAYRU, "Klippe bei der Hylia-Brücke", "Lake Hylia Bridge Cliff Poe", 0x06, 0x3B, true},
    {GROUP_LANAYRU, "Hylia-See: beim Turm", "Lake Hylia Tower Poe", 0x04, 0x4C, true},
    {GROUP_LANAYRU, "Laternenhöhle am Hylia-See: 1. Poe", "Lake Lantern Cave First Poe", 0x1A, 0x5E, false},
    {GROUP_LANAYRU, "Laternenhöhle am Hylia-See: 2. Poe", "Lake Lantern Cave Second Poe", 0x1A, 0x5D, false},
    {GROUP_LANAYRU, "Laternenhöhle am Hylia-See: letzter Poe", "Lake Lantern Cave Final Poe", 0x1A, 0x5F, false},
    {GROUP_LANAYRU, "Hylia-See: Felsnische am Ufer", "Lake Hylia Alcove Poe", 0x04, 0x46, true},
    {GROUP_LANAYRU, "Hylia-See: Insel der Reichtümer (Hühnerflug)", "Isle of Riches Poe", 0x04, 0x47, true},
    {GROUP_LANAYRU, "Hylia-See: Felsvorsprung beim Hühnerflug-Spiel", "Flight By Fowl Ledge Poe", 0x04, 0x4D, true},
    {GROUP_LANAYRU, "Hylia-See: am Steg", "Lake Hylia Dock Poe", 0x04, 0x4B, true},
    {GROUP_LANAYRU, "Oberer Zora-Fluss", "Upper Zoras River Poe", 0x04, 0x48, true},
    {GROUP_LANAYRU, "Zoras Reich: Mutter-und-Kind-Felsen", "Zoras Domain Mother and Child Isle Poe", 0x04, 0x4A, true},
    {GROUP_LANAYRU, "Zoras Reich: beim Wasserfall", "Zoras Domain Waterfall Poe", 0x04, 0x49, true},

    // --- Schneeberg ---
    {GROUP_SNOWPEAK, "Schneeberg: im Schneesturm-Gebiet", "Snowpeak Blizzard Poe", 0x08, 0x7D, true},
    {GROUP_SNOWPEAK, "Schneeberg: oberhalb der Eisgegner-Grotte", "Snowpeak Above Freezard Grotto Poe", 0x08, 0x7C, true},
    {GROUP_SNOWPEAK, "Schneeberg: zwischen den Bäumen", "Snowpeak Poe Among Trees", 0x08, 0x7B, true},
    {GROUP_SNOWPEAK, "Schneeberg: in der Höhle, im Eis", "Snowpeak Cave Ice Poe", 0x08, 0x7F, false},
    {GROUP_SNOWPEAK, "Schneeberg: eisiger Gipfel", "Snowpeak Icy Summit Poe", 0x08, 0x7E, true},

    // --- Gerudo-Wüste ---
    {GROUP_DESERT, "Gerudo-Wüste: Osten", "Gerudo Desert East Poe", 0x0A, 0x5C, true},
    {GROUP_DESERT, "Gerudo-Wüste: über dem Eingang der Drillhöhle", "Gerudo Desert Poe Above Cave of Ordeals", 0x0A, 0x5D, true},
    {GROUP_DESERT, "Gerudo-Wüste: Norden, bei den Peahats", "Gerudo Desert North Peahat Poe", 0x0A, 0x5B, true},
    {GROUP_DESERT, "Gerudo-Wüste: Felsen-Grotte, 1. Poe", "Gerudo Desert Rock Grotto First Poe", 0x1B, 0x0F, false},
    {GROUP_DESERT, "Gerudo-Wüste: Felsen-Grotte, 2. Poe", "Gerudo Desert Rock Grotto Second Poe", 0x1B, 0x10, false},
    {GROUP_DESERT, "Gerudo-Wüste: vor dem Bulblin-Lager", "Outside Bulblin Camp Poe", 0x0A, 0x33, true},
    {GROUP_DESERT, "Bulblin-Lager", "Bulblin Camp Poe", 0x0A, 0x78, true},
    {GROUP_DESERT, "Vor dem Eingang der Wüstenburg", "Outside Arbiters Grounds Poe", 0x0A, 0x5A, true},
    {GROUP_DESERT, "Drillhöhle: Ebene 17", "Cave of Ordeals Floor 17 Poe", 0x19, 0x45, false},
    {GROUP_DESERT, "Drillhöhle: Ebene 33", "Cave of Ordeals Floor 33 Poe", 0x19, 0x46, false},
    {GROUP_DESERT, "Drillhöhle: Ebene 44", "Cave of Ordeals Floor 44 Poe", 0x19, 0x47, false},

    // --- Dungeons ---
    {GROUP_DUNGEON, "Wüstenburg: Fackelraum (1. Poe)", "Arbiters Grounds Torch Room Poe", 0x13, 0x1E, false},
    {GROUP_DUNGEON, "Wüstenburg: östlicher Drehraum", "Arbiters Grounds East Turning Room Poe", 0x13, 0x1F, false},
    {GROUP_DUNGEON, "Wüstenburg: hinter der versteckten Wand", "Arbiters Grounds Hidden Wall Poe", 0x13, 0x20, false},
    {GROUP_DUNGEON, "Wüstenburg: Westflügel", "Arbiters Grounds West Poe", 0x13, 0x21, false},
    {GROUP_DUNGEON, "Bergruine: Eingangshalle, in der Rüstung", "Snowpeak Ruins Lobby Armor Poe", 0x14, 0x15, false},
    {GROUP_DUNGEON, "Bergruine: Eingangshalle", "Snowpeak Ruins Lobby Poe", 0x14, 0x72, false},
    {GROUP_DUNGEON, "Bergruine: Eisraum", "Snowpeak Ruins Ice Room Poe", 0x14, 0x7F, false},
    {GROUP_DUNGEON, "Zeitschrein: hinter dem Gitter", "Temple of Time Poe Behind Gate", 0x15, 0x19, false},
    {GROUP_DUNGEON, "Zeitschrein: über den Waagen", "Temple of Time Poe Above Scales", 0x15, 0x18, false},
    {GROUP_DUNGEON, "Kumula: Garteninsel", "City in the Sky Garden Island Poe", 0x16, 0x54, false},
    {GROUP_DUNGEON, "Kumula: über dem zentralen Ventilator", "City in the Sky Poe Above Central Fan", 0x16, 0x55, false},
};
// clang-format on

constexpr int kPoeCount = static_cast<int>(sizeof(kPoes) / sizeof(kPoes[0]));
static_assert(kPoeCount == 60, "es gibt genau 60 Poes");

// Dusklights eingebauter Kartenzaehler prueft fuer Jovanis Haus Flag 0x1F statt 0x1E.
constexpr uint8_t kJovaniSaveId = 0x09;
constexpr uint8_t kJovaniSwitch = 0x1E;
constexpr uint8_t kJovaniMapCounterSwitch = 0x1F;

// Speicherbereich der aktuell geladenen Stage; deren Flags liegen im Arbeitsspeicher und
// werden erst beim Verlassen in den Spielstand zurueckgeschrieben.
int current_save_id() {
    stage_stag_info_class* stag = dComIfGp_getStageStagInfo();
    if (stag == nullptr) {
        return -1;
    }
    return dStage_stagInfo_GetSaveTbl(stag);
}

bool is_switch(int currentSaveId, uint8_t saveId, uint8_t sw) {
    if (currentSaveId == saveId) {
        return dComIfGs_isSwitch(sw, -1) != 0;
    }
    return dComIfGs_isSaveSwitch(saveId, sw) != 0;
}

struct State {
    uint64_t collected = 0; // Bit i = kPoes[i] eingesammelt
    int counter = 0;        // Zaehler im Spielstand (Jovani)
    bool jovaniMismatch = false;

    bool operator==(const State&) const = default;

    bool has(int i) const { return (collected >> i) & 1u; }
    int count() const {
        int n = 0;
        for (int i = 0; i < kPoeCount; ++i) {
            n += has(i) ? 1 : 0;
        }
        return n;
    }
};

State read_state() {
    State s;
    const int cur = current_save_id();
    for (int i = 0; i < kPoeCount; ++i) {
        if (is_switch(cur, kPoes[i].saveId, kPoes[i].sw)) {
            s.collected |= uint64_t{1} << i;
        }
    }
    s.counter = dComIfGs_getPohSpiritNum();
    s.jovaniMismatch = is_switch(cur, kJovaniSaveId, kJovaniSwitch) !=
                       is_switch(cur, kJovaniSaveId, kJovaniMapCounterSwitch);
    return s;
}

void append_escaped(std::string& out, const char* text) {
    for (const char* p = text; *p != '\0'; ++p) {
        switch (*p) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        default:
            out += *p;
            break;
        }
    }
}

void append_row(std::string& out, const State& s, int i, bool withGroup) {
    const Poe& poe = kPoes[i];
    const bool have = s.has(i);
    out += have ? "<span style=\"color: #7fd68a;\">[x]</span> " :
                  "<span style=\"color: #ff7b6b;\">[FEHLT]</span> ";
    if (withGroup) {
        append_escaped(out, kGroupNames[poe.group]);
        out += " - ";
    }
    append_escaped(out, poe.where);
    if (poe.nightOnly) {
        out += " (nur nachts)";
    }
    out += " <span style=\"color: #9a9a9a;\">";
    append_escaped(out, poe.english);
    out += "</span>";
    if (poe.saveId == kJovaniSaveId && poe.sw == kJovaniSwitch && s.jovaniMismatch) {
        out += " <span style=\"color: #e8c15a;\">(Achtung: der Kartenzähler von Dusklight "
               "wertet hier ein anderes Flag aus)</span>";
    }
    out += "<br/>";
}

std::string summary_rml(const State& s) {
    const int have = s.count();
    std::string out;
    out += "Laut Spielstand-Flags eingesammelt: <b>" + std::to_string(have) + " / 60</b><br/>";
    out += "Seelen-Zähler (Jovani): <b>" + std::to_string(s.counter) + "</b><br/>";
    if (have != s.counter) {
        out += "<span style=\"color: #e8c15a;\">Zähler und Flags weichen voneinander ab "
               "(z. B. durch Cheats/Save-Editor oder einen noch nicht geladenen "
               "Spielstand).</span><br/>";
    }
    out += "<span style=\"color: #9a9a9a;\">Poes sieht man nur als Wolf mit geschärften "
           "Sinnen. Im Freien erscheinen sie nur nachts.</span>";
    return out;
}

std::string missing_rml(const State& s) {
    std::string out;
    for (int i = 0; i < kPoeCount; ++i) {
        if (!s.has(i)) {
            append_row(out, s, i, true);
        }
    }
    if (out.empty()) {
        out = "<span style=\"color: #7fd68a;\">Keine - alle 60 Poes sind eingesammelt.</span>";
    }
    return out;
}

std::string group_rml(const State& s, Group group) {
    std::string out;
    for (int i = 0; i < kPoeCount; ++i) {
        if (kPoes[i].group == group) {
            append_row(out, s, i, false);
        }
    }
    return out;
}

std::string group_title(const State& s, Group group) {
    int have = 0;
    int total = 0;
    for (int i = 0; i < kPoeCount; ++i) {
        if (kPoes[i].group == group) {
            ++total;
            have += s.has(i) ? 1 : 0;
        }
    }
    return std::string(kGroupNames[group]) + "  " + std::to_string(have) + " / " +
           std::to_string(total);
}

// Element-Handles des Panels; nur bis zum naechsten Neuaufbau gueltig.
UiElementHandle g_summary = 0;
UiElementHandle g_missing = 0;
UiElementHandle g_groupCount[GROUP_COUNT] = {};
UiElementHandle g_groupRows[GROUP_COUNT] = {};
State g_shown;

void apply_state(const State& s) {
    svc_ui->elem_set_rml(mod_ctx, g_summary, summary_rml(s).c_str());
    svc_ui->elem_set_rml(mod_ctx, g_missing, missing_rml(s).c_str());
    for (int g = 0; g < GROUP_COUNT; ++g) {
        const Group group = static_cast<Group>(g);
        svc_ui->elem_set_text(mod_ctx, g_groupCount[g], group_title(s, group).c_str());
        svc_ui->elem_set_rml(mod_ctx, g_groupRows[g], group_rml(s, group).c_str());
    }
    g_shown = s;
}

void show_missing_toast(ModContext*, void*) {
    const State s = read_state();
    std::string body;
    int shown = 0;
    int missing = 0;
    for (int i = 0; i < kPoeCount; ++i) {
        if (s.has(i)) {
            continue;
        }
        ++missing;
        if (shown < 6) {
            append_escaped(body, kGroupNames[kPoes[i].group]);
            body += " - ";
            append_escaped(body, kPoes[i].where);
            if (kPoes[i].nightOnly) {
                body += " (nur nachts)";
            }
            body += "<br/>";
            ++shown;
        }
    }
    if (missing == 0) {
        body = "Alle 60 Poes sind eingesammelt.";
    } else if (missing > shown) {
        body += "... und " + std::to_string(missing - shown) + " weitere";
    }
    const std::string title = "Fehlende Poes: " + std::to_string(missing);

    UiToastDesc toast = UI_TOAST_DESC_INIT;
    toast.title_rml = title.c_str();
    toast.body_rml = body.c_str();
    toast.duration_ms = 15000;
    svc_ui->push_toast(mod_ctx, &toast);
}

ModResult build_panel(ModContext*, UiElementHandle panel, void*, ModError*) {
    svc_ui->pane_add_section(mod_ctx, panel, "Übersicht");
    svc_ui->pane_add_rml(mod_ctx, panel, "", &g_summary);

    UiControlDesc button = UI_CONTROL_DESC_INIT;
    button.kind = UI_CONTROL_BUTTON;
    button.label = "Fehlende Poes als Hinweis im Spiel einblenden";
    button.on_pressed = show_missing_toast;
    svc_ui->pane_add_control(mod_ctx, panel, &button, nullptr);

    svc_ui->pane_add_section(mod_ctx, panel, "Fehlende Poes");
    svc_ui->pane_add_rml(mod_ctx, panel, "", &g_missing);

    svc_ui->pane_add_section(mod_ctx, panel, "Alle Poes nach Gebiet");
    for (int g = 0; g < GROUP_COUNT; ++g) {
        svc_ui->pane_add_text(mod_ctx, panel, "", &g_groupCount[g]);
        svc_ui->pane_add_rml(mod_ctx, panel, "", &g_groupRows[g]);
    }

    apply_state(read_state());
    return MOD_OK;
}

ModResult update_panel(ModContext*, void*, ModError*) {
    const State s = read_state();
    if (!(s == g_shown)) {
        apply_state(s);
    }
    return MOD_OK;
}

} // namespace

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError*) {
    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build_panel;
    panel.update = update_panel;
    const ModResult result = svc_ui->register_mods_panel(mod_ctx, &panel);
    if (result != MOD_OK) {
        return result;
    }
    svc_log->info(mod_ctx, "Poe Finder geladen");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
