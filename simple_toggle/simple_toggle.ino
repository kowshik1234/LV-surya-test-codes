/*
  Arduino code to toggle pins based on a menu selection.

  Macros:
  #define IR_850 2
  #define BLUE 42
  #define RED 40
  #define WHITE 38
  #define GREEN 36
  #define VIOLET 16
  #define IR_950 7
  #define Far_red_740 5
*/

// Define the pins using macros (as provided)
#define IR_850 2
#define BLUE 42
#define RED 40
#define WHITE 38
#define GREEN 36
#define VIOLET 16
#define IR_950 7
#define Far_red_740 5

#define LM3409 22


void setup() {
  // Initialize serial communication for printing the menu and getting user input
  Serial.begin(115200);

  // Set the pin modes for all the pins defined in the macros as OUTPUT
  pinMode(IR_850, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(WHITE, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(VIOLET, OUTPUT);
  pinMode(IR_950, OUTPUT);
  pinMode(Far_red_740, OUTPUT);
  pinMode(LM3409, OUTPUT);


  printMenu(); // Print the initial menu
}

void loop() {
  // Check if there is any data available from the serial port
  if (Serial.available() > 0) {
    // Read the incoming byte (character)
    int selection = Serial.parseInt(); // Read the integer typed by the user

    // Process the user's selection
    switch (selection) {
      case 1:
        togglePin(IR_850);
        break;
      case 2:
        togglePin(BLUE);
        break;
      case 3:
        togglePin(RED);
        break;
      case 4:
        togglePin(WHITE);
        break;
      case 5:
        togglePin(GREEN);
        break;
      case 6:
        togglePin(VIOLET);
        break;
      case 7:
        togglePin(IR_950);
        break;
      case 8:
        togglePin(Far_red_740);
        break;
      case 9:
        togglePin(LM3409);
        break;
      default:
        Serial.println("Invalid Selection");
        break;
    }
    printMenu(); // Print the menu again after processing the input
  }
}

// Function to toggle a pin (change its state)
void togglePin(int pin) {
  digitalWrite(pin, !digitalRead(pin)); // Toggles the pin's state
  Serial.print("Toggled pin: ");
  Serial.println(pin);
}

// Function to print the menu options
void printMenu() {
  Serial.println("Select a pin to toggle:");
  Serial.println("1. IR_850 (Pin 2)");
  Serial.println("2. BLUE (Pin 42)");
  Serial.println("3. RED (Pin 40)");
  Serial.println("4. WHITE (Pin 38)");
  Serial.println("5. GREEN (Pin 36)");
  Serial.println("6. VIOLET (Pin 16)");
  Serial.println("7. IR_950 (Pin 7)");
  Serial.println("8. Far_red_740 (Pin 5)");
  Serial.println("9. LM3409 (Pin 22)");

  Serial.println("Enter the number of the pin:");
}
