// Poe Finder - listet alle 60 Geisterseelen (Poe Souls) sowie alle 45 Herzteile und
// 8 Herzcontainer aus Twilight Princess mit Fundort auf und zeigt anhand der
// Spielstand-Flags, welche noch fehlen.
//
// Die Poe-Flags stammen aus Dusklights eigener Kartenzaehler-Tabelle (d_menu_map_common.cpp)
// und wurden mit dem Dusklight-Randomizer (locations.yaml) abgeglichen; die Herz-Flags
// stammen aus derselben Randomizer-Datei.

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
    GROUP_ORDONA,
    GROUP_FARON,
    GROUP_ELDIN,
    GROUP_LANAYRU,
    GROUP_SNOWPEAK,
    GROUP_DESERT,
    GROUP_DUNGEON,
    GROUP_COUNT,
};

const char* const kGroupNames[GROUP_COUNT] = {
    "Ordon",
    "Phirone (Faron)",
    "Eldin",
    "Ranelle (Lanayru)",
    "Schneeberg (Snowpeak)",
    "Gerudo-Wüste",
    "Dungeons",
};

// Art des Spielstand-Flags, das beim Einsammeln gesetzt wird.
enum FlagKind : uint8_t {
    FLAG_SWITCH, // Schalter-Flag der Region (Poes)
    FLAG_TBOX,   // Truhen-Flag der Region
    FLAG_ITEM,   // Flag eines frei liegenden Gegenstands der Region (0x80..0xBF)
    FLAG_EVENT,  // globales Ereignis-Flag (Byte << 8 | Bitmaske)
};

const char* const kTagNight = "nur nachts";
const char* const kTagContainer = "ganzer Herzcontainer";

struct Entry {
    Group group;
    const char* where;   // deutsche Ortsbeschreibung
    const char* english; // Name wie im Randomizer / in englischen Guides
    FlagKind kind;
    uint8_t saveId; // Speicherbereich (Region); bei FLAG_EVENT unbenutzt
    uint16_t flag;
    const char* tag; // optionaler Zusatz in Klammern
};

// clang-format off
const Entry kPoes[] = {
    // --- Phirone ---
    {GROUP_FARON, "Wald von Phirone: im Nebelgebiet", "Faron Mist Poe", FLAG_SWITCH, 0x02, 0x005D, kTagNight},
    {GROUP_FARON, "Hyrule-Ebene (Phirone)", "Faron Field Poe", FLAG_SWITCH, 0x06, 0x0039, kTagNight},
    {GROUP_FARON, "Verlorene Wälder: hinter dem Felsbrocken", "Lost Woods Boulder Poe", FLAG_SWITCH, 0x07, 0x0010, kTagNight},
    {GROUP_FARON, "Verlorene Wälder: beim Wasserfall", "Lost Woods Waterfall Poe", FLAG_SWITCH, 0x07, 0x0011, kTagNight},
    {GROUP_FARON, "Heiliger Hain: beim Master-Schwert-Podest", "Sacred Grove Master Sword Poe", FLAG_SWITCH, 0x07, 0x000F, kTagNight},
    {GROUP_FARON, "Heiliger Hain: bei der Eulenstatue vor dem Zeitschrein", "Sacred Grove Temple of Time Owl Statue Poe", FLAG_SWITCH, 0x07, 0x001E, kTagNight},

    // --- Eldin ---
    {GROUP_ELDIN, "Hyrule-Ebene (Eldin): Kakariko-Schlucht", "Kakariko Gorge Poe", FLAG_SWITCH, 0x06, 0x003A, kTagNight},
    {GROUP_ELDIN, "Laternenhöhle bei der Kakariko-Schlucht", "Eldin Lantern Cave Poe", FLAG_SWITCH, 0x19, 0x0060, nullptr},
    {GROUP_ELDIN, "Kakariko: beim Bombenladen", "Kakariko Village Bomb Shop Poe", FLAG_SWITCH, 0x03, 0x005E, kTagNight},
    {GROUP_ELDIN, "Kakariko: beim Wachturm", "Kakariko Village Watchtower Poe", FLAG_SWITCH, 0x03, 0x005F, kTagNight},
    {GROUP_ELDIN, "Friedhof von Kakariko: offen auf dem Friedhof", "Kakariko Graveyard Open Poe", FLAG_SWITCH, 0x03, 0x0058, kTagNight},
    {GROUP_ELDIN, "Friedhof von Kakariko: hinter/unter einem Grabstein", "Kakariko Graveyard Grave Poe", FLAG_SWITCH, 0x03, 0x0057, kTagNight},
    {GROUP_ELDIN, "Todesberg-Pfad", "Death Mountain Trail Poe", FLAG_SWITCH, 0x03, 0x0059, kTagNight},
    {GROUP_ELDIN, "Vergessenes Dorf", "Hidden Village Poe", FLAG_SWITCH, 0x03, 0x0040, kTagNight},

    // --- Ranelle ---
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): bei der Brücke", "Lanayru Field Bridge Poe", FLAG_SWITCH, 0x06, 0x0033, kTagNight},
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): Poe-Grotte, linker Poe", "Lanayru Field Poe Grotto Left Poe", FLAG_SWITCH, 0x1B, 0x000B, nullptr},
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): Poe-Grotte, rechter Poe", "Lanayru Field Poe Grotto Right Poe", FLAG_SWITCH, 0x1B, 0x000A, nullptr},
    {GROUP_LANAYRU, "Hyrule-Ebene: Amphitheater-Ruine bei Hyrule-Stadt", "Hyrule Field Amphitheater Poe", FLAG_SWITCH, 0x06, 0x0049, kTagNight},
    {GROUP_LANAYRU, "Brücke östlich von Hyrule-Stadt", "East Castle Town Bridge Poe", FLAG_SWITCH, 0x06, 0x0047, kTagNight},
    {GROUP_LANAYRU, "Hyrule-Stadt: in Jovanis Haus", "Jovani House Poe", FLAG_SWITCH, 0x09, 0x001E, nullptr},
    {GROUP_LANAYRU, "Vor dem Südtor von Hyrule-Stadt", "Outside South Castle Town Poe", FLAG_SWITCH, 0x06, 0x0030, kTagNight},
    {GROUP_LANAYRU, "Klippe bei der Hylia-Brücke", "Lake Hylia Bridge Cliff Poe", FLAG_SWITCH, 0x06, 0x003B, kTagNight},
    {GROUP_LANAYRU, "Hylia-See: beim Turm", "Lake Hylia Tower Poe", FLAG_SWITCH, 0x04, 0x004C, kTagNight},
    {GROUP_LANAYRU, "Laternenhöhle am Hylia-See: 1. Poe", "Lake Lantern Cave First Poe", FLAG_SWITCH, 0x1A, 0x005E, nullptr},
    {GROUP_LANAYRU, "Laternenhöhle am Hylia-See: 2. Poe", "Lake Lantern Cave Second Poe", FLAG_SWITCH, 0x1A, 0x005D, nullptr},
    {GROUP_LANAYRU, "Laternenhöhle am Hylia-See: letzter Poe", "Lake Lantern Cave Final Poe", FLAG_SWITCH, 0x1A, 0x005F, nullptr},
    {GROUP_LANAYRU, "Hylia-See: Felsnische am Ufer", "Lake Hylia Alcove Poe", FLAG_SWITCH, 0x04, 0x0046, kTagNight},
    {GROUP_LANAYRU, "Hylia-See: Insel der Reichtümer (Hühnerflug)", "Isle of Riches Poe", FLAG_SWITCH, 0x04, 0x0047, kTagNight},
    {GROUP_LANAYRU, "Hylia-See: Felsvorsprung beim Hühnerflug-Spiel", "Flight By Fowl Ledge Poe", FLAG_SWITCH, 0x04, 0x004D, kTagNight},
    {GROUP_LANAYRU, "Hylia-See: am Steg", "Lake Hylia Dock Poe", FLAG_SWITCH, 0x04, 0x004B, kTagNight},
    {GROUP_LANAYRU, "Oberer Zora-Fluss", "Upper Zoras River Poe", FLAG_SWITCH, 0x04, 0x0048, kTagNight},
    {GROUP_LANAYRU, "Zoras Reich: Mutter-und-Kind-Felsen", "Zoras Domain Mother and Child Isle Poe", FLAG_SWITCH, 0x04, 0x004A, kTagNight},
    {GROUP_LANAYRU, "Zoras Reich: beim Wasserfall", "Zoras Domain Waterfall Poe", FLAG_SWITCH, 0x04, 0x0049, kTagNight},

    // --- Schneeberg ---
    {GROUP_SNOWPEAK, "Schneeberg: im Schneesturm-Gebiet", "Snowpeak Blizzard Poe", FLAG_SWITCH, 0x08, 0x007D, kTagNight},
    {GROUP_SNOWPEAK, "Schneeberg: oberhalb der Eisgegner-Grotte", "Snowpeak Above Freezard Grotto Poe", FLAG_SWITCH, 0x08, 0x007C, kTagNight},
    {GROUP_SNOWPEAK, "Schneeberg: zwischen den Bäumen", "Snowpeak Poe Among Trees", FLAG_SWITCH, 0x08, 0x007B, kTagNight},
    {GROUP_SNOWPEAK, "Schneeberg: in der Höhle, im Eis", "Snowpeak Cave Ice Poe", FLAG_SWITCH, 0x08, 0x007F, nullptr},
    {GROUP_SNOWPEAK, "Schneeberg: eisiger Gipfel", "Snowpeak Icy Summit Poe", FLAG_SWITCH, 0x08, 0x007E, kTagNight},

    // --- Gerudo-Wüste ---
    {GROUP_DESERT, "Gerudo-Wüste: Osten", "Gerudo Desert East Poe", FLAG_SWITCH, 0x0A, 0x005C, kTagNight},
    {GROUP_DESERT, "Gerudo-Wüste: über dem Eingang der Drillhöhle", "Gerudo Desert Poe Above Cave of Ordeals", FLAG_SWITCH, 0x0A, 0x005D, kTagNight},
    {GROUP_DESERT, "Gerudo-Wüste: Norden, bei den Peahats", "Gerudo Desert North Peahat Poe", FLAG_SWITCH, 0x0A, 0x005B, kTagNight},
    {GROUP_DESERT, "Gerudo-Wüste: Felsen-Grotte, 1. Poe", "Gerudo Desert Rock Grotto First Poe", FLAG_SWITCH, 0x1B, 0x000F, nullptr},
    {GROUP_DESERT, "Gerudo-Wüste: Felsen-Grotte, 2. Poe", "Gerudo Desert Rock Grotto Second Poe", FLAG_SWITCH, 0x1B, 0x0010, nullptr},
    {GROUP_DESERT, "Gerudo-Wüste: vor dem Bulblin-Lager", "Outside Bulblin Camp Poe", FLAG_SWITCH, 0x0A, 0x0033, kTagNight},
    {GROUP_DESERT, "Bulblin-Lager", "Bulblin Camp Poe", FLAG_SWITCH, 0x0A, 0x0078, kTagNight},
    {GROUP_DESERT, "Vor dem Eingang der Wüstenburg", "Outside Arbiters Grounds Poe", FLAG_SWITCH, 0x0A, 0x005A, kTagNight},
    {GROUP_DESERT, "Drillhöhle: Ebene 17", "Cave of Ordeals Floor 17 Poe", FLAG_SWITCH, 0x19, 0x0045, nullptr},
    {GROUP_DESERT, "Drillhöhle: Ebene 33", "Cave of Ordeals Floor 33 Poe", FLAG_SWITCH, 0x19, 0x0046, nullptr},
    {GROUP_DESERT, "Drillhöhle: Ebene 44", "Cave of Ordeals Floor 44 Poe", FLAG_SWITCH, 0x19, 0x0047, nullptr},

    // --- Dungeons ---
    {GROUP_DUNGEON, "Wüstenburg: Fackelraum (1. Poe)", "Arbiters Grounds Torch Room Poe", FLAG_SWITCH, 0x13, 0x001E, nullptr},
    {GROUP_DUNGEON, "Wüstenburg: östlicher Drehraum", "Arbiters Grounds East Turning Room Poe", FLAG_SWITCH, 0x13, 0x001F, nullptr},
    {GROUP_DUNGEON, "Wüstenburg: hinter der versteckten Wand", "Arbiters Grounds Hidden Wall Poe", FLAG_SWITCH, 0x13, 0x0020, nullptr},
    {GROUP_DUNGEON, "Wüstenburg: Westflügel", "Arbiters Grounds West Poe", FLAG_SWITCH, 0x13, 0x0021, nullptr},
    {GROUP_DUNGEON, "Bergruine: Eingangshalle, in der Rüstung", "Snowpeak Ruins Lobby Armor Poe", FLAG_SWITCH, 0x14, 0x0015, nullptr},
    {GROUP_DUNGEON, "Bergruine: Eingangshalle", "Snowpeak Ruins Lobby Poe", FLAG_SWITCH, 0x14, 0x0072, nullptr},
    {GROUP_DUNGEON, "Bergruine: Eisraum", "Snowpeak Ruins Ice Room Poe", FLAG_SWITCH, 0x14, 0x007F, nullptr},
    {GROUP_DUNGEON, "Zeitschrein: hinter dem Gitter", "Temple of Time Poe Behind Gate", FLAG_SWITCH, 0x15, 0x0019, nullptr},
    {GROUP_DUNGEON, "Zeitschrein: über den Waagen", "Temple of Time Poe Above Scales", FLAG_SWITCH, 0x15, 0x0018, nullptr},
    {GROUP_DUNGEON, "Kumula: Garteninsel", "City in the Sky Garden Island Poe", FLAG_SWITCH, 0x16, 0x0054, nullptr},
    {GROUP_DUNGEON, "Kumula: über dem zentralen Ventilator", "City in the Sky Poe Above Central Fan", FLAG_SWITCH, 0x16, 0x0055, nullptr},
};

const Entry kHearts[] = {
    {GROUP_ORDONA, "Ordon-Ranch: Belohnung fürs Ziegenhüten bei Fado", "Herding Goats Reward", FLAG_EVENT, 0x00, 0x4240, nullptr},
    {GROUP_FARON, "Wald von Phirone: Höhle im Nebelgebiet, Truhe (Fackeln anzünden)", "Faron Mist Cave Lantern Chest", FLAG_TBOX, 0x02, 0x001A, nullptr},
    {GROUP_FARON, "Wald von Phirone: Truhe bei der Eulenstatue", "Faron Woods Owl Statue Chest", FLAG_TBOX, 0x02, 0x001E, nullptr},
    {GROUP_FARON, "Hyrule-Ebene (Phirone): Herzteil auf einem Baum", "Faron Field Tree Heart Piece", FLAG_ITEM, 0x06, 0x0081, nullptr},
    {GROUP_FARON, "Heiliger Hain: Grotte mit Schlangen-Dekuranhas, Truhe", "Sacred Grove Baba Serpent Grotto Chest", FLAG_TBOX, 0x1B, 0x0003, nullptr},
    {GROUP_FARON, "Heiliger Hain (Vergangenheit): Truhe bei der Eulenstatue", "Sacred Grove Past Owl Statue Chest", FLAG_TBOX, 0x07, 0x0001, nullptr},
    {GROUP_ELDIN, "Kakariko-Schlucht: Herzteil auf der Felssäule", "Kakariko Gorge Spire Heart Piece", FLAG_ITEM, 0x06, 0x0082, nullptr},
    {GROUP_ELDIN, "Kakariko-Schlucht: Truhe, nur mit Doppel-Greifhaken erreichbar", "Kakariko Gorge Double Clawshot Chest", FLAG_TBOX, 0x06, 0x0005, nullptr},
    {GROUP_ELDIN, "Laternenhöhle bei der Kakariko-Schlucht: Truhe (Fackeln anzünden)", "Eldin Lantern Cave Lantern Chest", FLAG_TBOX, 0x19, 0x003E, nullptr},
    {GROUP_ELDIN, "Kakariko: Truhe unter Wasser in der Geisterquelle", "Eldin Spring Underwater Chest", FLAG_TBOX, 0x03, 0x0015, nullptr},
    {GROUP_ELDIN, "Kakariko: Herzteil auf der Felsspitze hinter dem Sprengfelsen", "Kakariko Village Bomb Rock Spire Heart Piece", FLAG_ITEM, 0x03, 0x008C, nullptr},
    {GROUP_ELDIN, "Kakariko: Taros Bogenschiess-Spiel am Wachturm", "Talo Sharpshooting", FLAG_EVENT, 0x00, 0x0920, nullptr},
    {GROUP_ELDIN, "Todesberg-Pfad: Truhe in der Felsnische", "Death Mountain Alcove Chest", FLAG_TBOX, 0x03, 0x0016, nullptr},
    {GROUP_ELDIN, "Hyrule-Ebene (Eldin): Truhe hinter einem Sprengfelsen", "Eldin Field Bomb Rock Chest", FLAG_TBOX, 0x06, 0x0002, nullptr},
    {GROUP_ELDIN, "Hyrule-Ebene: heisses Quellwasser rechtzeitig zum Goronen bringen", "Goron Springwater Rush", FLAG_ITEM, 0x06, 0x0080, nullptr},
    {GROUP_ELDIN, "Eldin-Brücke: Truhe bei der Eulenstatue", "Bridge of Eldin Owl Statue Chest", FLAG_TBOX, 0x06, 0x0003, nullptr},
    {GROUP_ELDIN, "Höhle bei der Eldin-Brücke: unterste Truhe", "Eldin Stockcave Lowest Chest", FLAG_TBOX, 0x1A, 0x003E, nullptr},
    {GROUP_ELDIN, "Hyrule-Ebene: Stalfos-Grotte, Truhe nach dem Kampf", "Eldin Field Stalfos Grotto Stalfos Chest", FLAG_TBOX, 0x1B, 0x0002, nullptr},
    {GROUP_ELDIN, "Vergessenes Dorf: Katzen-Versteckspiel", "Cats Hide and Seek Minigame", FLAG_ITEM, 0x03, 0x008B, nullptr},
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): Höhle mit dem Eisblock-Rätsel, Truhe", "Lanayru Ice Block Puzzle Cave Chest", FLAG_TBOX, 0x19, 0x0000, nullptr},
    {GROUP_LANAYRU, "Hyrule-Ebene (Ranelle): Truhe an der Gleiter-Schiene", "Lanayru Field Spinner Track Chest", FLAG_TBOX, 0x06, 0x0008, nullptr},
    {GROUP_LANAYRU, "Hyrule-Stadt: dem Priester insgesamt 1000 Rubine spenden", "Charlo Donation Blessing", FLAG_EVENT, 0x00, 0x2480, nullptr},
    {GROUP_LANAYRU, "Laternenhöhle am Hylia-See: Truhe am Ende (Fackeln anzünden)", "Lake Lantern Cave End Lantern Chest", FLAG_TBOX, 0x1A, 0x0003, nullptr},
    {GROUP_LANAYRU, "Hylia-See: Hühnerflug, Truhe auf der zweiten Plattform", "Flight By Fowl Second Platform Chest", FLAG_TBOX, 0x04, 0x0000, nullptr},
    {GROUP_LANAYRU, "Hylia-See: Geisterquelle, hinterer Raum, Truhe (Fackeln anzünden)", "Lanayru Spring Back Room Lantern Chest", FLAG_TBOX, 0x04, 0x001D, nullptr},
    {GROUP_LANAYRU, "Hylia-See: Plumms Fruchtballon-Spiel", "Plumm Fruit Balloon Minigame", FLAG_EVENT, 0x00, 0x2380, nullptr},
    {GROUP_LANAYRU, "Angelteich: Herzteil auf dem Felsen (mit der Angel holen)", "Fishing Hole Heart Piece", FLAG_ITEM, 0x0B, 0x0080, nullptr},
    {GROUP_SNOWPEAK, "Schneeberg: Belohnung fürs Schlittenrennen", "Snowboard Racing Prize", FLAG_EVENT, 0x00, 0x3B10, nullptr},
    {GROUP_DESERT, "Bulblin-Lager: im gebratenen Wildschwein", "Bulblin Camp Roasted Boar", FLAG_ITEM, 0x0A, 0x009F, nullptr},
    {GROUP_DUNGEON, "Waldschrein: Westen, Truhe bei der Deku-Schlinger-Pflanze", "Forest Temple West Deku Like Chest", FLAG_TBOX, 0x10, 0x001F, nullptr},
    {GROUP_DUNGEON, "Waldschrein: Westen, Truhe hinter der Treppe (Plattenwurm-Raum)", "Forest Temple West Tile Worm Chest Behind Stairs", FLAG_TBOX, 0x10, 0x0013, nullptr},
    {GROUP_DUNGEON, "Waldschrein: Herzcontainer nach dem Boss (Diababa)", "Forest Temple Diababa Heart Container", FLAG_ITEM, 0x10, 0x009F, kTagContainer},
    {GROUP_DUNGEON, "Goronen-Mine: Truhe im Magnet-Labyrinth", "Goron Mines Magnet Maze Chest", FLAG_TBOX, 0x11, 0x0014, nullptr},
    {GROUP_DUNGEON, "Goronen-Mine: Truhe an der Magnetwand nach dem Kristallschalter-Raum", "Goron Mines After Crystal Switch Room Magnet Wall Chest", FLAG_TBOX, 0x11, 0x0009, nullptr},
    {GROUP_DUNGEON, "Goronen-Mine: Herzcontainer nach dem Boss (Fyrus)", "Goron Mines Fyrus Heart Container", FLAG_ITEM, 0x11, 0x009F, kTagContainer},
    {GROUP_DUNGEON, "Seeschrein: Truhe auf dem Kronleuchter", "Lakebed Temple Chandelier Chest", FLAG_TBOX, 0x12, 0x0005, nullptr},
    {GROUP_DUNGEON, "Seeschrein: Osten, Truhe bei der Brücke am unteren Wasserrad", "Lakebed Temple East Lower Waterwheel Bridge Chest", FLAG_TBOX, 0x12, 0x0008, nullptr},
    {GROUP_DUNGEON, "Seeschrein: Herzcontainer nach dem Boss (Morpheel)", "Lakebed Temple Morpheel Heart Container", FLAG_ITEM, 0x12, 0x009F, kTagContainer},
    {GROUP_DUNGEON, "Wüstenburg: Fackelraum, östliche Truhe", "Arbiters Grounds Torch Room East Chest", FLAG_TBOX, 0x13, 0x0013, nullptr},
    {GROUP_DUNGEON, "Wüstenburg: Gleiter-Raum, Truhe in der Stalfos-Nische", "Arbiters Grounds Spinner Room Stalfos Alcove Chest", FLAG_TBOX, 0x13, 0x001A, nullptr},
    {GROUP_DUNGEON, "Wüstenburg: Herzcontainer nach dem Boss (Stallord)", "Arbiters Grounds Stallord Heart Container", FLAG_ITEM, 0x13, 0x009F, kTagContainer},
    {GROUP_DUNGEON, "Bergruine: Eingangshalle, Truhe auf dem Kronleuchter", "Snowpeak Ruins Lobby Chandelier Chest", FLAG_TBOX, 0x14, 0x0015, nullptr},
    {GROUP_DUNGEON, "Bergruine: Truhe im Raum mit dem eingebrochenen Boden", "Snowpeak Ruins Broken Floor Chest", FLAG_TBOX, 0x14, 0x0019, nullptr},
    {GROUP_DUNGEON, "Bergruine: Herzcontainer nach dem Boss (Blizzeta)", "Snowpeak Ruins Blizzeta Heart Container", FLAG_ITEM, 0x14, 0x009F, kTagContainer},
    {GROUP_DUNGEON, "Zeitschrein: Armos-Vorraum, Truhe (mit Statue erreichbar)", "Temple of Time Armos Antechamber Statue Chest", FLAG_TBOX, 0x15, 0x0005, nullptr},
    {GROUP_DUNGEON, "Zeitschrein: Dinalfos-Raum mit der beweglichen Wand, Truhe", "Temple of Time Moving Wall Dinalfos Room Chest", FLAG_TBOX, 0x15, 0x000C, nullptr},
    {GROUP_DUNGEON, "Zeitschrein: Herzcontainer nach dem Boss (Armogohma)", "Temple of Time Armogohma Heart Container", FLAG_ITEM, 0x15, 0x009F, kTagContainer},
    {GROUP_DUNGEON, "Kumula: Dekuranha-Turm, Truhe in der Nische", "City in the Sky Baba Tower Alcove Chest", FLAG_TBOX, 0x16, 0x0006, nullptr},
    {GROUP_DUNGEON, "Kumula: Westgarten, Truhe auf dem Vorsprung", "City in the Sky West Garden Ledge Chest", FLAG_TBOX, 0x16, 0x0009, nullptr},
    {GROUP_DUNGEON, "Kumula: Herzcontainer nach dem Boss (Argorok)", "City in the Sky Argorok Heart Container", FLAG_ITEM, 0x16, 0x009F, kTagContainer},
    {GROUP_DUNGEON, "Schattenpalast: Westflügel, Truhe hinter der Wand aus Finsternis", "Palace of Twilight West Wing Chest Behind Wall of Darkness", FLAG_TBOX, 0x17, 0x001E, nullptr},
    {GROUP_DUNGEON, "Schattenpalast: Ostflügel, erster Raum, Truhe in der östlichen Nische", "Palace of Twilight East Wing First Room East Alcove Chest", FLAG_TBOX, 0x17, 0x0000, nullptr},
    {GROUP_DUNGEON, "Schattenpalast: Herzcontainer nach dem Boss (Zanto)", "Palace of Twilight Zant Heart Container", FLAG_ITEM, 0x17, 0x0080, kTagContainer},
};
// clang-format on

constexpr int kPoeCount = static_cast<int>(sizeof(kPoes) / sizeof(kPoes[0]));
constexpr int kHeartCount = static_cast<int>(sizeof(kHearts) / sizeof(kHearts[0]));
static_assert(kPoeCount == 60, "es gibt genau 60 Poes");
static_assert(kHeartCount == 53, "45 Herzteile + 8 Herzcontainer");

constexpr int kPieceTotal = 45;
constexpr int kContainerTotal = 8;

// Dusklights eingebauter Kartenzaehler prueft fuer Jovanis Haus Flag 0x1F statt 0x1E.
constexpr uint8_t kJovaniSaveId = 0x09;
constexpr uint16_t kJovaniSwitch = 0x1E;
constexpr uint16_t kJovaniMapCounterSwitch = 0x1F;

// Speicherbereich der aktuell geladenen Stage; deren Flags liegen im Arbeitsspeicher und
// werden erst beim Verlassen in den Spielstand zurueckgeschrieben.
int current_save_id() {
    stage_stag_info_class* stag = dComIfGp_getStageStagInfo();
    if (stag == nullptr) {
        return -1;
    }
    return dStage_stagInfo_GetSaveTbl(stag);
}

bool is_flag_set(int currentSaveId, FlagKind kind, uint8_t saveId, uint16_t flag) {
    const bool live = currentSaveId == saveId;
    switch (kind) {
    case FLAG_SWITCH:
        return live ? dComIfGs_isSwitch(flag, -1) != 0 : dComIfGs_isSaveSwitch(saveId, flag) != 0;
    case FLAG_TBOX:
        return live ? dComIfGs_isTbox(flag) != 0 : dComIfGs_isSaveTbox(saveId, flag) != 0;
    case FLAG_ITEM:
        if (live) {
            return dComIfGs_isItem(flag, -1);
        }
        // Gespeicherte Regionen zaehlen die Item-Flags ab 0 statt ab 0x80.
        return g_dComIfG_gameInfo.info.getSavedata().getSave(saveId).getBit().isItem(flag - 0x80) !=
               0;
    case FLAG_EVENT:
        return dComIfGs_isEventBit(flag) != 0;
    }
    return false;
}

struct Category {
    const Entry* entries;
    int count;
    const char* missingTitle;
    const char* allTitle;
    const char* noneMissing;
};

enum CategoryId { CAT_POES, CAT_HEARTS, CAT_COUNT };

const Category kCategories[CAT_COUNT] = {
    {kPoes, kPoeCount, "Fehlende Poes", "Alle Poes nach Gebiet",
        "Keine - alle 60 Poes sind eingesammelt."},
    {kHearts, kHeartCount, "Fehlende Herzteile und Herzcontainer",
        "Alle Herzteile und Herzcontainer nach Gebiet",
        "Keine - alle 45 Herzteile und 8 Herzcontainer sind eingesammelt."},
};

struct State {
    uint64_t collected[CAT_COUNT] = {}; // Bit i = Eintrag i eingesammelt
    int poeCounter = 0;                 // Seelen-Zaehler im Spielstand (Jovani)
    int maxLife = 0;                    // Herzleiste in Fuenfteln (15 = 3 Herzen)
    bool jovaniMismatch = false;

    bool operator==(const State&) const = default;

    bool has(int cat, int i) const { return ((collected[cat] >> i) & 1u) != 0; }
    int count(int cat) const {
        int n = 0;
        for (int i = 0; i < kCategories[cat].count; ++i) {
            n += has(cat, i) ? 1 : 0;
        }
        return n;
    }
    int containers() const {
        int n = 0;
        for (int i = 0; i < kHeartCount; ++i) {
            n += (has(CAT_HEARTS, i) && kHearts[i].tag == kTagContainer) ? 1 : 0;
        }
        return n;
    }
};

State read_state() {
    State s;
    const int cur = current_save_id();
    for (int cat = 0; cat < CAT_COUNT; ++cat) {
        const Category& category = kCategories[cat];
        for (int i = 0; i < category.count; ++i) {
            const Entry& e = category.entries[i];
            if (is_flag_set(cur, e.kind, e.saveId, e.flag)) {
                s.collected[cat] |= uint64_t{1} << i;
            }
        }
    }
    s.poeCounter = dComIfGs_getPohSpiritNum();
    s.maxLife = dComIfGs_getMaxLife();
    s.jovaniMismatch = is_flag_set(cur, FLAG_SWITCH, kJovaniSaveId, kJovaniSwitch) !=
                       is_flag_set(cur, FLAG_SWITCH, kJovaniSaveId, kJovaniMapCounterSwitch);
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

void append_place(std::string& out, const Entry& e, bool withGroup) {
    if (withGroup) {
        append_escaped(out, kGroupNames[e.group]);
        out += " - ";
    }
    append_escaped(out, e.where);
    if (e.tag != nullptr) {
        out += " (";
        append_escaped(out, e.tag);
        out += ")";
    }
}

void append_row(std::string& out, const State& s, int cat, int i, bool withGroup) {
    const Entry& e = kCategories[cat].entries[i];
    out += s.has(cat, i) ? "<span style=\"color: #7fd68a;\">[x]</span> " :
                           "<span style=\"color: #ff7b6b;\">[FEHLT]</span> ";
    append_place(out, e, withGroup);
    out += " <span style=\"color: #9a9a9a;\">";
    append_escaped(out, e.english);
    out += "</span>";
    if (cat == CAT_POES && e.saveId == kJovaniSaveId && e.flag == kJovaniSwitch &&
        s.jovaniMismatch)
    {
        out += " <span style=\"color: #e8c15a;\">(Achtung: der Kartenzähler von Dusklight "
               "wertet hier ein anderes Flag aus)</span>";
    }
    out += "<br/>";
}

std::string summary_rml(const State& s) {
    const int poes = s.count(CAT_POES);
    const int containers = s.containers();
    const int pieces = s.count(CAT_HEARTS) - containers;
    const int expectedLife = 15 + 5 * containers + pieces;

    std::string out;
    out += "<b>Poes</b> laut Spielstand-Flags: <b>" + std::to_string(poes) + " / 60</b>";
    out += " - Seelen-Zähler (Jovani): <b>" + std::to_string(s.poeCounter) + "</b><br/>";
    if (poes != s.poeCounter) {
        out += "<span style=\"color: #e8c15a;\">Seelen-Zähler und Flags weichen voneinander "
               "ab.</span><br/>";
    }
    out += "<b>Herzteile</b>: <b>" + std::to_string(pieces) + " / " +
           std::to_string(kPieceTotal) + "</b> - <b>Herzcontainer</b>: <b>" +
           std::to_string(containers) + " / " + std::to_string(kContainerTotal) + "</b><br/>";
    out += "Herzleiste im Spielstand: <b>" + std::to_string(s.maxLife / 5) + " Herzen</b>";
    if (s.maxLife % 5 != 0) {
        out += " + " + std::to_string(s.maxLife % 5) + " von 5 Herzteilen";
    }
    out += "<br/>";
    if (expectedLife != s.maxLife) {
        out += "<span style=\"color: #e8c15a;\">Herzleiste und Flags weichen voneinander ab "
               "(z. B. noch kein Spielstand geladen, Cheats/Save-Editor oder "
               "Randomizer).</span><br/>";
    }
    out += "<span style=\"color: #9a9a9a;\">Poes sieht man nur als Wolf mit geschärften "
           "Sinnen. Im Freien erscheinen sie nur nachts.</span>";
    return out;
}

std::string missing_rml(const State& s, int cat) {
    std::string out;
    for (int i = 0; i < kCategories[cat].count; ++i) {
        if (!s.has(cat, i)) {
            append_row(out, s, cat, i, true);
        }
    }
    if (out.empty()) {
        out = "<span style=\"color: #7fd68a;\">";
        out += kCategories[cat].noneMissing;
        out += "</span>";
    }
    return out;
}

bool group_used(int cat, int group) {
    for (int i = 0; i < kCategories[cat].count; ++i) {
        if (kCategories[cat].entries[i].group == group) {
            return true;
        }
    }
    return false;
}

std::string group_rml(const State& s, int cat, int group) {
    std::string out;
    for (int i = 0; i < kCategories[cat].count; ++i) {
        if (kCategories[cat].entries[i].group == group) {
            append_row(out, s, cat, i, false);
        }
    }
    return out;
}

std::string group_title(const State& s, int cat, int group) {
    int have = 0;
    int total = 0;
    for (int i = 0; i < kCategories[cat].count; ++i) {
        if (kCategories[cat].entries[i].group == group) {
            ++total;
            have += s.has(cat, i) ? 1 : 0;
        }
    }
    return std::string(kGroupNames[group]) + "  " + std::to_string(have) + " / " +
           std::to_string(total);
}

// Element-Handles des Panels; nur bis zum naechsten Neuaufbau gueltig.
UiElementHandle g_summary = 0;
UiElementHandle g_missing[CAT_COUNT] = {};
UiElementHandle g_groupCount[CAT_COUNT][GROUP_COUNT] = {};
UiElementHandle g_groupRows[CAT_COUNT][GROUP_COUNT] = {};
State g_shown;

void apply_state(const State& s) {
    svc_ui->elem_set_rml(mod_ctx, g_summary, summary_rml(s).c_str());
    for (int cat = 0; cat < CAT_COUNT; ++cat) {
        svc_ui->elem_set_rml(mod_ctx, g_missing[cat], missing_rml(s, cat).c_str());
        for (int g = 0; g < GROUP_COUNT; ++g) {
            if (!group_used(cat, g)) {
                continue;
            }
            svc_ui->elem_set_text(
                mod_ctx, g_groupCount[cat][g], group_title(s, cat, g).c_str());
            svc_ui->elem_set_rml(mod_ctx, g_groupRows[cat][g], group_rml(s, cat, g).c_str());
        }
    }
    g_shown = s;
}

void show_missing_toast(ModContext*, void*) {
    const State s = read_state();
    std::string body;
    int shown = 0;
    int missing = 0;
    for (int cat = 0; cat < CAT_COUNT; ++cat) {
        for (int i = 0; i < kCategories[cat].count; ++i) {
            if (s.has(cat, i)) {
                continue;
            }
            ++missing;
            if (shown < 6) {
                body += cat == CAT_POES ? "Poe: " : "Herz: ";
                append_place(body, kCategories[cat].entries[i], true);
                body += "<br/>";
                ++shown;
            }
        }
    }
    if (missing == 0) {
        body = "Alle Poes, Herzteile und Herzcontainer sind eingesammelt.";
    } else if (missing > shown) {
        body += "... und " + std::to_string(missing - shown) + " weitere";
    }
    const std::string title = "Noch offen: " + std::to_string(missing);

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
    button.label = "Fehlendes als Hinweis im Spiel einblenden";
    button.on_pressed = show_missing_toast;
    svc_ui->pane_add_control(mod_ctx, panel, &button, nullptr);

    for (int cat = 0; cat < CAT_COUNT; ++cat) {
        svc_ui->pane_add_section(mod_ctx, panel, kCategories[cat].missingTitle);
        svc_ui->pane_add_rml(mod_ctx, panel, "", &g_missing[cat]);
    }

    for (int cat = 0; cat < CAT_COUNT; ++cat) {
        svc_ui->pane_add_section(mod_ctx, panel, kCategories[cat].allTitle);
        for (int g = 0; g < GROUP_COUNT; ++g) {
            if (!group_used(cat, g)) {
                continue;
            }
            svc_ui->pane_add_text(mod_ctx, panel, "", &g_groupCount[cat][g]);
            svc_ui->pane_add_rml(mod_ctx, panel, "", &g_groupRows[cat][g]);
        }
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
