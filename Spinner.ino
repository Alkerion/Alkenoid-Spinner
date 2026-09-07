/***********************************************************
* SPINNER ALKENOID - deux exemplaires, deux joueurs
*----------------------------------------------------------*
* Arduino Pro Micro (ATmega32U4), a televerser en carte
* "Arduino Leonardo" - meme bootloader, meme USB natif.
* Bibliotheque : HID-Project (gestionnaire de bibliotheques).
*
* UNE MANETTE, ET NON UNE SOURIS.
*
* LE PRIX A PAYER, ET COMMENT ON LE PAIE : un axe de manette
* est ABSOLU et borne (-32768 a +32767), alors qu'un spinner
* tourne sans fin.
*
* LE CABLAGE
*   encodeur A (blanc) -> pin 2      encodeur GND -> GND
*   encodeur B (vert)  -> pin 3      encodeur VCC -> VCC (pont J1 à souder pour avoir 5V)
*   bouton 1           -> pin 4, l'autre patte a la masse
*   bouton 2           -> pin 5, idem
*   bouton 3           -> pin 6, idem
*
* TROIS BOUTONS PAR SPINNER, et le meme firmware sur les deux
* cartes. Ils sont separes, pas en parallele : le jeu voit
* trois boutons distincts et leur donne trois roles.
*
* LES BOUTONS N'ONT PAS DE RESISTANCE EXTERNE : INPUT_PULLUP
* branche celle du microcontroleur, la patte est donc a 1 au
* repos et tombe a 0 quand on appuie. Un fil, une masse, rien
* d'autre a souder.
***********************************************************/

#include <HID-Project.h>

static const uint8_t PIN_A = 2;   // INT1 = PD1
static const uint8_t PIN_B = 3;   // INT0 = PD0
static const uint8_t PIN_BOUTON[] = { 4, 5, 6 };
static const uint8_t NB_BOUTONS = sizeof(PIN_BOUTON);
static const uint8_t BOUTON_MANETTE[] = { 1, 2, 3 };
static const int8_t SENS = -1;
static const uint8_t DIVISEUR = 1;
static const uint8_t ANTIREBOND = 5;

static const int8_t PAS[16] = {
	 0, -1, +1,  0,
	+1,  0,  0, -1,
	-1,  0,  0, +1,
	 0, +1, -1,  0
};

volatile int32_t compteur = 0;
volatile uint8_t precedent = 0;

// ---------------------------------------------------------

static inline uint8_t etatEncodeur() {
	uint8_t p = PIND;
	return (uint8_t)(((p >> 1) & 1) << 1 | (p & 1));   // A puis B
}

// ---------------------------------------------------------

void tourne() {
	uint8_t maintenant = etatEncodeur();
	compteur += SENS * PAS[(precedent << 2) | maintenant];
	precedent = maintenant;
}

// ---------------------------------------------------------

void setup() {
	pinMode(PIN_A, INPUT_PULLUP);
	pinMode(PIN_B, INPUT_PULLUP);
	for (uint8_t i = 0; i < NB_BOUTONS; i++) {
		pinMode(PIN_BOUTON[i], INPUT_PULLUP);
	}

	precedent = etatEncodeur();

	attachInterrupt(digitalPinToInterrupt(PIN_A), tourne, CHANGE);
	attachInterrupt(digitalPinToInterrupt(PIN_B), tourne, CHANGE);

	Gamepad.begin();
}


// ---------------------------------------------------------

void loop() {

	static uint32_t dernier = 0;
	uint32_t t = millis();
	if (t == dernier) {
		return;
	}
	dernier = t;

	noInterrupts();
	int32_t c = compteur;
	interrupts();

	int16_t axe = (int16_t)(c / (int32_t)DIVISEUR);

	static bool etat[NB_BOUTONS] = { false, false, false };
	static bool vu[NB_BOUTONS] = { false, false, false };
	static uint32_t depuis[NB_BOUTONS] = { 0, 0, 0 };
	for (uint8_t i = 0; i < NB_BOUTONS; i++) {
		bool brut = (digitalRead(PIN_BOUTON[i]) == LOW);
		if (brut != vu[i]) {
			vu[i] = brut;
			depuis[i] = t;
		}
		else if (brut != etat[i] && (t - depuis[i]) >= ANTIREBOND) {
			etat[i] = brut;
			if (etat[i]) { Gamepad.press(BOUTON_MANETTE[i]); }
			else { Gamepad.release(BOUTON_MANETTE[i]); }
		}
	}

	static int16_t axePose = 0;
	static uint8_t boutonsPoses = 0;
	uint8_t masque = 0;
	for (uint8_t i = 0; i < NB_BOUTONS; i++) {
		if (etat[i]) { masque |= (uint8_t)(1 << i); }
	}
	if (axe != axePose || masque != boutonsPoses) {
		Gamepad.xAxis(axe);
		Gamepad.write();
		axePose = axe;
		boutonsPoses = masque;
	}
}