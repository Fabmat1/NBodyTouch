#pragma once

enum class LKey {
    StarProperties,
    MassSun,
    SimSpeed,
    Play,
    Pause,
    Clear,
    StarsCount,
    TouchDragHint,
    ControlsHint,
    BackToApp,
    HRDiagram,
    Hot,
    Cool,
    ZoomIn,
    ZoomOut,
    Zoom,
    Temperature,
    Bright,
    Dim,
    Luminosity,
    OutsideSpawnArea,
    COUNT
};

struct Language {
    const char *code;
    const char *name;
    const char *flagFile;
    const char *strings[(int)LKey::COUNT];
};

inline const Language LANGUAGES[] = {
    {
        "de", "Deutsch", "de.png",
        {
            /*StarProperties*/   "Sterneigenschaften",
            /*MassSun*/          "Masse (Sonnenmassen)",
            /*SimSpeed*/         "Simulationsgeschwindigkeit",
            /*Play*/             "Start",
            /*Pause*/            "Pause",
            /*Clear*/            "Löschen",
            /*StarsCount*/       "Sterne: %d / %d",
            /*TouchDragHint*/    "Berühren & ziehen für neuen Stern",
            /*ControlsHint*/     "2-Finger Zoom - 3-Finger Schwenken",
            /*BackToApp*/        "Zurück zur Appübersicht",
            /*HRDiagram*/        "H-R Diagramm",
            /*Hot*/              "Heiß",
            /*Cool*/             "Kühl",
            /*ZoomIn*/           "+",
            /*ZoomOut*/          "-",
            /*Zoom*/             "Zoom",
            /*Temperature*/      "Temperatur",
            /*Bright*/           "Hell",
            /*Dim*/              "Dunkel",
            /*Luminosity*/       "Helligkeit",
            /*OutsideSpawnArea*/ "Außerhalb des erlaubten Bereichs",
        }
    },
    {
        "en", "English", "en.png",
        {
            /*StarProperties*/    "Star Properties",
            /*MassSun*/           "Mass (Solar Masses)",
            /*SimSpeed*/          "Simulation Speed",
            /*Play*/              "Play",
            /*Pause*/             "Pause",
            /*Clear*/             "Clear",
            /*StarsCount*/        "Stars: %d / %d",
            /*TouchDragHint*/     "Touch & drag to place a star",
            /*ControlsHint*/      "Scroll/pinch zoom - 3-finger pan",
            /*BackToApp*/         "Back to App Overview",
            /*HRDiagram*/         "H-R Diagram",
            /*Hot*/               "Hot",
            /*Cool*/              "Cool",
            /*ZoomIn*/            "+",
            /*ZoomOut*/           "-",
            /*Zoom*/              "Zoom",
            /*Temperature*/       "Temperature",
            /*Bright*/            "Bright",
            /*Dim*/               "Dim",
            /*Luminosity*/        "Luminosity",
            /*OutsideSpawnArea*/  "Outside spawn area",
        }
    },
    {
    "es", "Español", "es.png",
        {
            /*StarProperties*/   "Propiedades de la estrella",
            /*MassSun*/          "Masa (masas solares)",
            /*SimSpeed*/         "Velocidad de simulación",
            /*Play*/             "Iniciar",
            /*Pause*/            "Pausar",
            /*Clear*/            "Limpiar",
            /*StarsCount*/       "Estrellas: %d / %d",
            /*TouchDragHint*/    "Toca y arrastra para colocar una estrella",
            /*ControlsHint*/     "Pellizca para zoom - 3 dedos para mover",
            /*BackToApp*/        "Volver a la vista general",
            /*HRDiagram*/        "Diagrama H-R",
            /*Hot*/              "Caliente",
            /*Cool*/             "Frío",
            /*ZoomIn*/           "+",
            /*ZoomOut*/          "-",
            /*Zoom*/             "Zoom",
            /*Temperature*/      "Temperatura",
            /*Bright*/           "Brillante",
            /*Dim*/              "Tenue",
            /*Luminosity*/       "Luminosidad",
            /*OutsideSpawnArea*/ "Fuera del área permitida",
        }
    },
};

inline constexpr int LANGUAGE_COUNT = sizeof(LANGUAGES) / sizeof(LANGUAGES[0]);