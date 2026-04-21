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
    ResetCamera,
    IntroPage1,
    IntroPage2,
    IntroPage3,
    IntroPage4,
    IntroPage5,
    IntroNext,
    IntroBack,
    IntroDone,
    Scenarios,
    ScenarioTriple,
    ScenarioFour,
    ScenarioFreefall,
    ScenarioInfinity,
    ScenarioRecursion,
    ScenarioPingPong,
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
            /*ResetCamera*/      "Ansicht zurücksetzen",
            /*IntroPage1*/
                "Willkommen zur N-Körper-Simulation!\n\n"
                "Berühre und ziehe auf dem Bildschirm, um einen Stern zu "
                "erstellen und ihm eine Geschwindigkeit zu geben.\n\n"
                "Wähle eine Masse für die Sterne im Seitenpanel aus.\n\n"
                "Wie sehen Sterne mit verschiedenen Massen aus und wie "
                "verhalten sie sich? Kannst du ein System aus drei oder "
                "mehr Sternen stabil umkreisen lassen?",
            /*IntroPage2*/
                "Verwende zwei Finger zum Zoomen oder Drehen der Ansicht.\n\n"
                "Verwende drei Finger zum Schwenken.",
            /*IntroPage3*/
                "Berühre hier, um die Simulationsgeschwindigkeit anzupassen, "
                "die Simulation zu pausieren oder die platzierten Sterne "
                "zurückzusetzen.",
            /*IntroPage4*/
                "Berühre hier, um den Zoom anzupassen und den "
                "Massenschwerpunkt der Sterne zu verfolgen.\n\n"
                "TIPP: Verfolge den Massenschwerpunkt, nachdem du 2 oder "
                "mehr Sterne platziert hast!",
            /*IntroPage5*/
                "Fertig mit der Simulation?\n\n"
                "Tippe hier, um zum Hauptmenü zurückzukehren!",
            /*IntroNext*/        "Weiter",
            /*IntroBack*/        "Zurück",
            /*IntroDone*/        "Fertig",
            /*Scenarios*/        "Szenarien",
            /*ScenarioTriple*/   "Triple Problem",
            /*ScenarioFour*/     "Fantastic Four",
            /*ScenarioFreefall*/ "Freier Fall",
            /*ScenarioInfinity*/ "Unendlichkeit",
            /*ScenarioRecursion*/"Rekursion",
            /*ScenarioPingPong*/ "Ping Pong",
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
            /*ResetCamera*/       "Reset View",
            /*IntroPage1*/
                "Welcome to the N-Body Simulation!\n\n"
                "Touch and drag somewhere on screen to create a star "
                "and give it some velocity.\n\n"
                "Select a mass for the stars you create on the side panel.\n\n"
                "How do stars with different masses look and behave? "
                "Can you make a system of three or more stars orbit stably?",
            /*IntroPage2*/
                "Use two fingers to zoom or rotate the view.\n\n"
                "Use three fingers to pan.",
            /*IntroPage3*/
                "Touch here to adjust the simulation speed, "
                "pause the simulation or reset the placed stars.",
            /*IntroPage4*/
                "Touch here to adjust the zoom and to track the mass center "
                "of the stars to get a better look.\n\n"
                "TIP: Make sure to track the mass center after you "
                "placed 2 or more stars!",
            /*IntroPage5*/
                "Done with the simulation?\n\n"
                "Tap here to get back to the main menu!",
            /*IntroNext*/         "Next",
            /*IntroBack*/         "Back",
            /*IntroDone*/         "Done",
            /*Scenarios*/        "Scenarios",
            /*ScenarioTriple*/   "Triple Problem",
            /*ScenarioFour*/     "Fantastic Four",
            /*ScenarioFreefall*/ "Freefall",
            /*ScenarioInfinity*/ "Infinity",
            /*ScenarioRecursion*/"Recursion",
            /*ScenarioPingPong*/ "Ping Pong",
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
            /*ResetCamera*/      "Restablecer vista",
            /*IntroPage1*/
                "¡Bienvenido a la simulación de N-Cuerpos!\n\n"
                "Toca y arrastra en la pantalla para crear una estrella "
                "y darle velocidad.\n\n"
                "Selecciona una masa en el panel lateral.\n\n"
                "¿Cómo se ven y se comportan estrellas con diferentes masas? "
                "¿Puedes crear un sistema estable de tres o más estrellas "
                "en órbita?",
            /*IntroPage2*/
                "Usa dos dedos para hacer zoom o girar la vista.\n\n"
                "Usa tres dedos para desplazar la vista.",
            /*IntroPage3*/
                "Toca aquí para ajustar la velocidad de simulación, "
                "pausar la simulación o reiniciar las estrellas.",
            /*IntroPage4*/
                "Toca aquí para ajustar el zoom y seguir el centro de masa "
                "de las estrellas.\n\n"
                "¡CONSEJO: Asegúrate de seguir el centro de masa después "
                "de colocar 2 o más estrellas!",
            /*IntroPage5*/
                "¿Terminaste con la simulación?\n\n"
                "¡Toca aquí para volver al menú principal!",
            /*IntroNext*/        "Siguiente",
            /*IntroBack*/        "Atrás",
            /*IntroDone*/        "Listo",
            /*Scenarios*/        "Escenarios",
            /*ScenarioTriple*/   "Problema Triple",
            /*ScenarioFour*/     "Los Cuatro",
            /*ScenarioFreefall*/ "Caída Libre",
            /*ScenarioInfinity*/ "Infinito",
            /*ScenarioRecursion*/"Recursión",
            /*ScenarioPingPong*/ "Ping Pong",
        }
    },
};

inline constexpr int LANGUAGE_COUNT = sizeof(LANGUAGES) / sizeof(LANGUAGES[0]);