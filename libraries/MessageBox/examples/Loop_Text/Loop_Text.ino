#define HARDWARE_LILYGO_T_BLOCK
// #define HARDWARE_LILYGO_T_H239
#include <MessageBoxHardware.h>
#include <TextDisplay.h>
// Makes dealing with buttons a little bit more sane
#include <Button2.h>

// The io port, the display, the graphic library, and our own text wrapper.
GxIO_Class io(SPI, /*CS=5*/ EINK_SS, /*DC=*/ EINK_DC, /*RST=*/ EINK_RESET);
GxEPD_Class e_paper(io, /*RST=*/ EINK_RESET, /*BUSY=*/ EINK_BUSY);
TextDisplay text_display(e_paper);

Button2 button = Button2(USER_BUTTON);


typedef char * MessageType;
static constexpr MessageType sample_messages[] = {
  "123",
  "Ceci n'est pas un message.",
  "Que ce passe t'il lorsqu'un texte très long est mis en place sans prévenir ?",
  "çéâêîôûàèìòùëïü É ß",
  "Votre goût a servi de règle à mon ouvrage. "
  "J’ai tenté les moyens d’acquérir son suffrage. "
  "Vous voulez qu’on évite un soin trop curieux, "
  "Et des vains ornements l’effort ambitieux ; "
  "Je le veux comme vous : cet effort ne peut plaire. "
  "Un auteur gâte tout quand il veut trop bien faire. "
  "Non qu’il faille bannir certains traits délicats : "
  "Vous les aimez, ces traits, et je ne les hais pas. "
  "Quant au principal but qu’Ésope se propose, "
  "J’y tombe au moins mal que je puis. "
  "Enfin, si dans ces vers, je ne plais et n’instruis, "
  "Il ne tient pas à moi ; c’est toujours quelque chose. "
  "Comme la force est un point "
  "Dont je ne me pique point, "
  "Je tâche d’y tourner le vice en ridicule, "
  "Ne pouvant l’attaquer avec des bras d’Hercule. "
  "C’est là tout mon talent ; je ne sais s’il suffit. "
  "Tantôt je peins en un récit "
  "La sotte vanité jointe avecque l’envie, "
  "Deux pivots sur qui roule aujourd’hui notre vie : "
  "Tel est ce chétif animal "
  "Qui voulut en grosseur au boeuf se rendre égal. "
  "J’oppose quelquefois, par une double image, "
  "Le vice à la vertu, la sottise au bon sens, "
  "Les agneaux aux loups ravissants, "
  "La mouche à la fourmi, faisant de cet ouvrage "
  "Une ample comédie à cent actes divers, "
  "Et dont la scène est l’Univers. "
  "Hommes, dieux, animaux, tout y fait quelque rôle, "
  "Jupiter comme un autre. Introduisons celui "
  "Qui porte de sa part aux belles la parole : "
  "Ce n’est pas de cela qu’il s’agit aujourd’hui. "
  "Un bûcheron perdit son gagne-pain, "
  "C’est sa cognée ; et la cherchant en vain, "
  "Ce fut pitié là-dessus de l’entendre. "
  "Il n’avait pas des outils à revendre : "
  "Sur celui-ci roulait tout son avoir. "
  "Ne sachant donc où mettre son espoir, "
  "Sa face était de pleurs toute baignée : "
  "« Ô ma cognée ! ô ma pauvre cognée ! "
  "S’écriait-il, Jupiter, rends-la-moi ; "
  "Je tiendrai l’être encore un coup de toi. » "
  "Sa plainte fut de l’Olympe entendue. "
  "Mercure vient. « Elle n’est pas perdue, "
  "Lui dit ce dieu, la connaîtrais-tu bien ? "
  "Je crois l’avoir près d’ici rencontrée. » "
  "Lors une d’or à l’homme étant montrée, "
  "Il répondit : « Je n’y demande rien. » "
  "Une d’argent succède à la première, "
  "Il la refuse. Enfin une de bois : "
  "« Voilà, dit-il, la mienne cette fois ; "
  "Je suis content si j’ai cette dernière. "
  "– Tu les auras, dit le Dieu, toutes trois. "
  "Ta bonne foi sera récompensée. "
  "– En ce cas-là je les prendrai », dit-il. "
  "L’histoire en est aussitôt dispersée ; "
  "Et boquillons de perdre leur outil, "
  "Et de crier pour se le faire rendre. "
  "Le roi des Dieux ne sait auquel entendre. "
  "Son fils Mercure aux criards vient encore ; "
  "À chacun d’eux il en montre une d’or. "
  "Chacun eût cru passer pour une bête "
  "De ne pas dire aussitôt : « La voilà ! » "
  "Mercure, au lieu de donner celle-là, "
  "Leur en décharge un grand coup sur la tête. "
  "Ne point mentir, être content du sien, "
  "C’est le plus sûr : cependant on s’occupe "
  "À dire faux pour attraper du bien. "
  "Que sert cela ? Jupiter n’est pas dupe. "
};
static constexpr int sample_messages_count = sizeof(sample_messages) / sizeof(MessageType);

static char *currentMessage = nullptr;

static void refreshScreen() {
  e_paper.fillScreen(GxEPD_WHITE);  
  text_display.update(currentMessage);
  e_paper.update();
}

// On button press, cycle the display.
static void press(Button2 &button) {
  static int messageIndex = -1;
  messageIndex = (messageIndex + 1) % sample_messages_count;
  currentMessage = sample_messages[messageIndex];
  refreshScreen();
}


void setup() {
	Serial.begin(115200);

  SPI.begin(EINK_SPI_CLK, EINK_SPI_MISO, EINK_SPI_MOSI, EINK_SS);
  e_paper.init();
  
  static const int margin = 3;
  
  text_display.setDisplayPosition(
      Rect(margin, margin,
           e_paper.width() - margin * 2, e_paper.height() -  margin * 2));
    
  currentMessage = "Press button to cycle";
  refreshScreen();
  button.setPressedHandler(press);
}

void loop() {
  button.loop();
}
