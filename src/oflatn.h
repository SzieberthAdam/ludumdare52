#ifndef __DEFINE_RORHFILE__
#define __DEFINE_RORHFILE__

#include <stdbool.h>
#include <stdint.h>


#define RORFILESIGN "RoR"
#define RORFILEVERS ((uint8_t)(1))

#define ITEMTYPECOUNT 8
#define TEMPSIZE 12

#define SOLITAIRE_FCNT 5
#define TWOPLAYERS_FCNT 5

// uint32_t range : 0 to 4.294.967.295

typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    uint8_t     yea;        //  the number of years since 1990 (AH RoR release).
    uint8_t     mon;
    uint8_t     day;
    uint8_t     hou;
    uint8_t     min;
    uint8_t     sec;
} RoR_Time_t;               //  1990-00-00 00:00:00 is a nullish value


enum GamePhase {
    SetupPhase = 0,
    MortalityPhase = 1,
    RevenuePhase = 2,
    ForumPhase = 3,
    PopulationPhase = 4,
    SenatePhase = 5,
    CombatPhase = 6,
    RevolutionPhase = 7
};


enum Phase {
    PhSetGamename           = (SetupPhase << 28)        +      1,
    PhSetupRules            = (SetupPhase << 28)        +    256,
    PhPickScenario          = (SetupPhase << 28)        + 300000,
    PhTakeFactions          = (SetupPhase << 28)        + 301200,
    PhDealSenators          = (SetupPhase << 28)        + 301420,
    PhDealFactionCards      = (SetupPhase << 28)        + 301430,
    PhTemporaryRomeConsul   = (SetupPhase << 28)        + 301600,
    PhSelectFactionLeaders  = (SetupPhase << 28)        + 301700,
    PhInitialFactionPhase   = (SetupPhase << 28)        + 301900
};


enum Language {
    English = 0
};


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        sign[3];
    uint8_t     vers;
    uint8_t     lang;
    char        name[15];
    char        desc[76];
    RoR_Time_t  strt;       //  start
    RoR_Time_t  end;
    uint16_t    turn;
    uint32_t    phse;       //  phase ((rulebook phase << 28) + subphase)
                            //  this value is the main logic control
    uint8_t     eran;       //  era number
    uint8_t     deck;       //  active draw deck nr
} RoR_Header_t;


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    uint16_t    cnt;
    uint32_t    adr;        //  address of the first elem
} RoR_ItemTypeInfo_t;


enum ItemType {
    EraItem = 1,
    FactionItem,
    LocationItem,
    DeckItem,
    OfficeItem,
    MagistrateItem,
    SenatorItem,
    StatesmanItem,
    ConcessionItem,
    IntrigueItem
};


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        name[16];
    uint16_t    trig;       //  era triggers at specific game turn (value in bit0..11)
                            //      bit12: 0:OR 1:AND
                            //      bit13..15=0 : NEVER  -- NOTE(Adam): should be IMMEDIATELY but that would break natural language logic
                            //      bit13..15=1 : deck with this era [0 card (with any=0 era) drawn from deck with this=1 era: 001]
                            //      bit13..15=5 : card is drawn from deck with this era [1 card (with any=0 era) drawn from deck with this=1 era: 101]
                            //      bit13..15=6 : card with this era is drawn [1 card (with this=1 era) drawn from deck with any=0 era: 110]
                            //      bit13..15=7 : card with this era is drawn from deck with this era [1 card (with this=1 era) drawn from deck with this=1 era: 111]
                            //  NOTE(Adam): for ERA end card, use value of 4096 (TURN 0 AND NEVER)
    uint8_t     nsen;       //  minimum number of senators to ensure at era start
    uint8_t     ncar;       //  minimum number of faction cards to ensure at era start
    uint8_t     terc;       //  temporary rome consul protocol at era start
                            //      bit0: no TERC (0) / TERC (1)
                            //      bit1: random draw (0) / lowest senator ID (1)
                            //      bit2: before(0) / after(1) faction leaders
                            //      bit3: repeat if dies in first mortality phase (1)
                            //      bit7: not resolved (0) / resolved (1)
    char        pad;
} RoR_EraItem_t;


enum FactionType {
    FactionSet     = 0x01,
    FactionUsed    = 0x02,
    FactionNeutral = 0x04
};


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        name[16];
    uint8_t     type;       //  See FactionType enum.
    uint8_t     open;       //  Open Treasury (bit0=1) and Cards(bit1=1)
    uint8_t     asor;       //  Assignment Order Value (0: will not get assigned; otherwise lower value first)
    uint8_t     ast1;       //  Assignment by this value first (value start from zero)
                            //      bit7: +total Military
                            //      bit6: +total Oratory
                            //      bit5: +total Loyalty
                            //      bit4: +total Influence
                            //      bit3: -total Military
                            //      bit2: -total Oratory
                            //      bit1: -total Loyalty
                            //      bit0: -total Influence
    uint8_t     ast2;       //  Assignment by this value second (first tiebreaker)
    uint8_t     ast3;       //  Assignment by this value third (second tiebreaker)
    uint8_t     ast4;       //  Assignment by this value fourth (third tiebreaker)
    uint8_t     terc;       //  (Temporary) Rome Consul (value=0 signs that faction does not claim the TERC)
                            //      bit7..4: priority (highest faction takes the office)
                            //      bit3: Military adds to senator weight (senator with highest weight wins; tie-breaker is the lower ID)
                            //      bit2: Oratory adds to senator weight
                            //      bit1: Loyalty adds to senator weight
                            //      bit0: Influence adds to senator weight
    uint8_t     xsen;       //  Number of extra senators to deal to the faction at era start
    uint8_t     ledr;       //  Faction Leader assignment (highest weight value wins it; tie-breaker is the lower ID)
                            //      bit0: automatic(1) / Owner or Dominant Player decides (0)
                            //      bit1=1: Military to weight value
                            //      bit2=1: Oratory to weight value
                            //      bit3=1: Loyalty to weight value
                            //      bit4=1: Influence to weight value
                            //      bit5=1: Popularity to weight value
                            //      bit6=1: Knights to weight value
                            //      bit7=1: Personal Treasury to weight value
    uint8_t     ibi0;       //  Initiative opening bid modifier
                            //      bit7: automatic(1) / Owner or Dominant Player decides(0)
                            //      bit6: turn number to value
                            //      bit5: 1d6 to value
                            //      bit0..4: (x-16) to value
    uint8_t     ibi1;       //  Initiative bid modifier
                            //      bit7: automatic(1) / Owner or Dominant Player decides(0)
                            //      bit6: turn number to value
                            //      bit5: 1d6 to value
                            //      bit0..4: (x-16) to value
    uint8_t     reve;       //  Revenue:
                            //      bit0: remainder to Faction Treasury(0)/Faction Leader(1)
                            //      bi1..7=0: Owner / Dominant Player decides
                            //      bi1..7=1: 1d6 to Faction Treasury
                            //      bi1..7=2: Divided equally among senators
                            //      bi1..7=3: 1/2 to Faction Treasury (rounded down)
                            //      bi1..7=4: 1 per Senator and Faction Treasury
    uint8_t     crty;       //  Charity:
                            //      bit0=1: Treasury
                            //      bit4=1: Games
    uint8_t     prsp;       //  Provincial Spoils:
                            //      0: Owner / Dominant Player decides
                            //      1: Always
                            //      2..6: (on 1d6>=2..6)
                            //      7: None
    uint8_t     spoi[6];    //  Spoils:
                            //      0: None
                            //      1: Rome Consul
                            //      2: Field Consul
                            //      3: Censor
                            //      4: Concession
                            //      5: Governor
                            //      6: Land Bill
                            //      Censor, Concessions, Governor, RC, FC
                            //      RC, Censor, FC, Governor, Concession
                            //      RC, FC, Censor, Concession, Land Bill, Governor
    uint8_t     knig;       //  Talents for Knight attraction
                            //      bit7: automatic(1) / Owner or Dominant Player decides(0)
                            //      bit0..6: amount
    int8_t      clim;       //  Card limit (negative value: no limit)
    bool        rulc;       //  In Ruling Coalition
    uint8_t     domi;       //  Dominance
    uint8_t     leit;       //  Faction Leader ItemType (0=Null / Senator=Family Card / Statesman)
    uint8_t     lenr;       //  Faction Leader ElemNr
    uint16_t    tmil;       //  total Military
    uint16_t    tora;       //  total Oratory
    uint16_t    tloy;       //  total Loyalty
    uint16_t    tinf;       //  total Influence
    int16_t     tpop;       //  total Popularity
    uint8_t     card;       //  Cards in hand
} RoR_FactionItem_t;


typedef struct __attribute__((__packed__)) {
    char        name[16];
    uint8_t     type;
    uint8_t     open;
    uint8_t     asor;
    uint8_t     ast1;
    uint8_t     ast2;
    uint8_t     ast3;
    uint8_t     ast4;
    uint8_t     terc;
    uint8_t     xsen;
    uint8_t     ledr;
    uint8_t     ibi0;
    uint8_t     ibi1;
    uint8_t     reve;
    uint8_t     crty;
    uint8_t     prsp;
    uint8_t     spoi[6];
    uint8_t     knig;
    int8_t      clim;
    bool        rulc;
    uint8_t     domi;
    uint8_t     leit;
    uint8_t     lenr;
    uint16_t    tmil;
    uint16_t    tora;
    uint16_t    tloy;
    uint16_t    tinf;
    int16_t     tpop;
    uint8_t     card;
} RoR_FactionItem_t_unordered;



enum LocationElemNr {
    Discards = 1,
    Forum = 2,
    EnemyLeaders = 3,
    RepopulatingRome = 4,
    DestroyedConcessions = 5,
    InactiveWars = 6,
    UnprosecutedWars = 7,
    ActiveWars = 8,
    ActiveForces = 9,
    Laws = 10,
    ImminentWars = 11
};


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        name[32];
    uint16_t    seri;       // serial
} RoR_LocationItem_t;


enum DeckType {
    Deck = 0,
    EraTransDeck = 1,
    FinalDeck = 2,
    EraStartSenatorPool = 3,
    EraStartFactionCardPool = 4
};


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    uint8_t     eran;       //  Era number
    uint8_t     type;       //  Deck type
    int16_t     size;
    uint8_t     prev;
    uint8_t     next;
    uint16_t    topr;       //  First N cards goes to previous deck
    uint16_t    tonx;       //  Last N cards goes to next deck
    uint16_t    draw;       //  Draw count
} RoR_DeckItem_t;


enum OfficeType {
    ConsulForLife = 1,
    Dictator = 2,
    RomeConsul = 3,
    FieldConsul = 4,
    Censor = 5,
    MasterOfHorse = 6,
    DominantPlayer = 7,
    PontifexMaximus = 8,
    Proconsul = 9
};


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        snam[4];    //  Short name eg. "RC" stands for Rome Consul
    char        name[24];   //  Name
    char        ordr[2];    //  Order of precedence mark
    uint8_t     type;
    bool        life;       //  For life
    int8_t      infl;       //  Influence gain for taking
    char        pad;
} RoR_OfficeItem_t;


enum MagistrateType {
    HRAO = 1,
    PresidingMagistrate = 2,
};


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        snam[2];    //  "H" / "M"
    char        name[6];    //  "HRAO", "PRMA"
    char        lame[36];   //  "Highest Ranking Available Officer", "Presiding Magistrate"
    uint8_t     type;
    uint16_t    owit;       //  Owner's ItemType (Senator / Statesman)
    uint16_t    ownr;       //  Owner's ElemNr
    char        pad;
} RoR_MagistrateItem_t;


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        name[16];
    uint8_t     eran;       //  Era number
    uint8_t     idnr;       //  Family ID number (#x)
    uint8_t     mil0;       //  initial Military
    uint8_t     ora0;       //  initial Oratory
    uint8_t     loy0;       //  initial Loyalty
    uint8_t     inf0;       //  initial Influence
    int8_t      pop0;       //  initial Popularity
    uint8_t     mil1;       //  achieved Military
    uint8_t     ora1;       //  achieved Oratory
    uint8_t     loy1;       //  achieved Loyalty
    uint8_t     inf1;       //  achieved Influence
    int8_t      pop1;       //  achieved Popularity
    uint8_t     mil2;       //  current Military
    uint8_t     ora2;       //  current Oratory
    uint8_t     loy2;       //  current Loyalty
    uint8_t     inf2;       //  current Influence
    int8_t      pop2;       //  current Popularity
    uint8_t     fact;       //  Aligned to Faction#
    uint16_t    loit;       //  Location ItemType (0: Null / Deck / Discard / Forum (In Rome) / Repopulation Rome / Province / War)
    uint16_t    lonr;       //  Location ElemNr
    uint16_t    losr;       //  Location SerialNr (Mainly for Repopulating Rome)
    uint8_t     offi;       //  Office
    uint8_t     pric;       //  Prior Consul counter
} RoR_SenatorItem_t;


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    uint16_t    itid;       //  Item ID
    char        snam[16];   //  Short name
    uint8_t     eran;       //  Era number
    uint8_t     idnr;       //  Family ID number (#x)
    char        idch[2];    //  ID character (#xA/#xB/..)
    char        name[64];   //  Name
    char        time[32];   //  Time
    char        spec[128];  //  Specialty
    uint8_t     mil0;       //  initial Military
    uint8_t     ora0;       //  initial Oratory
    uint8_t     loy0;       //  initial Loyalty
    uint8_t     inf0;       //  initial Influence
    int8_t      pop0;       //  initial Popularity
    uint8_t     mil1;       //  achieved Military
    uint8_t     ora1;       //  achieved Oratory
    uint8_t     loy1;       //  achieved Loyalty
    uint8_t     inf1;       //  achieved Influence
    int8_t      pop1;       //  achieved Popularity
    uint8_t     mil2;       //  current Military
    uint8_t     ora2;       //  current Oratory
    uint8_t     loy2;       //  current Loyalty
    uint8_t     inf2;       //  current Influence
    int8_t      pop2;       //  current Popularity
    uint8_t     fact;       //  Aligned to Faction#
    uint16_t    loit;       //  Location ItemType (0: Null / Deck / Discard / Forum (In Rome) / Repopulation Rome / Province / War)
    uint16_t    lonr;       //  Location ElemNr
    uint8_t     offi;       //  Office
    uint8_t     pric;       //  Prior Consul counter
    uint8_t     lo0w[4];    //  Loyalty=0 if in same Faction with StatesmanNr (StatesmanId+1); 0: None
    uint8_t     lo0n[2];    //  Loyalty=0 if NOT in same Faction with StatesmanNr (StatesmanId+1); 0: None
} RoR_StatesmanItem_t;


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        name[32];   //  Name
    uint8_t     eran;       //  Era number
    uint8_t     type;       //  Type
    char        typs[32];   //  Type string
    char        desc[512];  //  Description
    uint16_t    cnit;       //  Connected ItemType (0: Null / Deck / Discard / Forum (In Rome) / Repopulation Rome / Province / War)
    uint16_t    cnnr;       //  Connected ElemNr
    int16_t     valu;       //  Value
    uint16_t    loit;       //  Location ItemType (0: Null / Deck / Discard / Forum (In Rome) / Repopulation Rome / Province / War)
    uint16_t    lonr;       //  Location ElemNr
} RoR_ConcessionItem_t;


typedef struct __attribute__((__packed__, __scalar_storage_order__("big-endian"))) {
    char        name[32];   //  Name
    char        note[32];   //  Note
    uint8_t     eran;       //  Era number
    uint8_t     type;       //  Type
    char        typs[32];   //  Type string
    char        desc[512];  //  Description
    uint16_t    loit;       //  Location ItemType (0: Null / Deck / Discard / Forum (In Rome) / Repopulation Rome / Province / War)
    uint16_t    lonr;       //  Location ElemNr
} RoR_IntrigueItem_t;


#define p_HEADER(rordata)  ((RoR_Header_t*)(rordata))
#define p_TEMP(rordata)  ((void*)(rordata+sizeof(RoR_Header_t)))
#define p_ITEMTYPEINFO(rordata)  ((RoR_ItemTypeInfo_t*)(rordata+sizeof(RoR_Header_t)+TEMPSIZE))
#define p_ITEM(rordata, itemname)  ((RoR_ ## itemname ## Item_t*)(rordata+p_ITEMTYPEINFO(rordata)[itemname ## Item-1].adr))
#define ITEMCOUNT(rordata, itemname)  (p_ITEMTYPEINFO(rordata)[itemname ## Item-1].cnt)


#define HEADER          (p_HEADER(rordata)[0])
#define ERA(n)          (p_ITEM(rordata, Era        )[n-1])
#define FACTION(n)      (p_ITEM(rordata, Faction    )[n-1])
#define LOCATION(n)     (p_ITEM(rordata, Location   )[n-1])
#define DECK(n)         (p_ITEM(rordata, Deck       )[n-1])
#define OFFICE(n)       (p_ITEM(rordata, Office     )[n-1])
#define MAGISTRATE(n)   (p_ITEM(rordata, Magistrate )[n-1])
#define SENATOR(n)      (p_ITEM(rordata, Senator    )[n-1])
#define STATESMAN(n)    (p_ITEM(rordata, Statesman  )[n-1])
#define CONCESSION(n)   (p_ITEM(rordata, Concession )[n-1])
#define INTRIGUE(n)     (p_ITEM(rordata, Intrigue   )[n-1])


#endif  /* #ifndef __DEFINE_RORHFILE__ */
;