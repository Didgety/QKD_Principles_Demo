#define laser 2 // port 2 on arduino
#define sensor 3 // port 3 on arduino
unsigned long signalPeriod = 1000; //milliseconds
bool state; // 1: laser detected, 0: no laser detected

const unsigned int messageLength = 2; // num of bits in message
const unsigned int keyLength = 18; // num of transmissions with random basis (longer than message to ensure enough matching bases for a long enough key)

/* 
[+ basis: 0, 2], [x basis: 1, 3], [0 = 0 deg, 1 = 45 deg, 2 = 90 deg, 3 = -45 deg]
*/
int aliceBasisList[keyLength];
int bobBasisList[keyLength];

int aliceKeyBits[keyLength];

int laserDetected = 0;
int keyDetectionList[keyLength];

int validBases[messageLength];
int validKeys[messageLength];
int validIndex[messageLength];

int encryptedMessage[messageLength];
int receivedMessage[messageLength];
int decryptedMessage[messageLength];

// using 5 bit binary for simplification, 1 = A, 2 = B, etc.
// ref: https://www.binary-code.org/bits/5
int message[] = {0, 0, 0, 1, 0,
                 0, 1, 1, 1, 1,
                 0, 1, 0, 0, 0,
                 1, 0, 0, 1, 0}; //"BOHR"

void setup() {
  Serial.begin(9600); // 9600 baud

  pinMode(laser, OUTPUT);
  pinMode(sensor, INPUT);

  genBases();

  delay(signalPeriod); // give program time to think before printing, unusual artifacts in output otherwise
  // print initial values
  //[0 = 0 deg, 1 = 45 deg, 2 = 90 deg, 3 = -45 deg]
  Serial.print("\nAlice Bases: ");
  for(int i = 0; i < keyLength; i++) {
    switch (aliceBasisList[i]) {
      case 0:
        Serial.print("0 deg, ");
        break;
      case 1:
        Serial.print("45 deg, ");
        break;
      case 2:
        Serial.print("90 deg, ");
        break;
      case 3:
        Serial.print("-45 deg, ");
        break;
    }
  }
  Serial.print("\nBob Bases: ");
  for(int i = 0; i < keyLength; i++) {
    switch (bobBasisList[i]) {
      case 0:
        Serial.print("0 deg, ");
        break;
      case 1:
        Serial.print("45 deg, ");
        break;
    }
  }
  Serial.print("\nAlice Key Bits: ");
  for(int i = 0; i < keyLength; i++) {
    Serial.print(aliceKeyBits[i]);
    Serial.print(", ");
  }
}

void loop() {
  digitalWrite(laser, LOW); // start with laser off
  
  int numErrors = basisTest();

  bool validKey = keyGen();
  while (validKey == false) {
    Serial.println("Insufficient matching values for message size, regenerating");
    
    double errorRate = (double)(keyLength - numErrors) / (double)keyLength;

    Serial.print("\nAccuracy: ");
    Serial.println(errorRate);
    Serial.print("\nLaser detections: ");
    Serial.println(laserDetected);

    genBases();
    basisTest();

    // print new values
    Serial.print("New Alice Bases: ");
    for(int i = 0; i < keyLength; i++) {
      switch (aliceBasisList[i]) {
      case 0:
        Serial.print("0 deg, ");
        break;
      case 1:
        Serial.print("45 deg, ");
        break;
      case 2:
        Serial.print("90 deg, ");
        break;
      case 3:
        Serial.print("-45 deg, ");
        break;
		}
    }
    Serial.print("\nNew Bob Bases: ");
    for(int i = 0; i < keyLength; i++) {
      switch (bobBasisList[i]) {
      case 0:
        Serial.print("0 deg, ");
        break;
      case 1:
        Serial.print("45 deg, ");
        break;
		}
    }
    Serial.print("\nNew Alice Key Bits: ");
    for(int i = 0; i < keyLength; i++) {
      Serial.print(aliceKeyBits[i]);
      Serial.print(", ");
    }
  }

  double errorRate = (double)(keyLength - numErrors) / (double)keyLength;
  
  Serial.print("\nAccuracy: ");
  Serial.println(errorRate);
  Serial.print("\nLaser detections: ");
  Serial.println(laserDetected);
  // print the bits generated
  Serial.print("Bits generated: ");
  for (int i = 0; i < keyLength; i++) {
    Serial.print(keyDetectionList[i]);
    Serial.print(", ");
  }
  // print the index value of valid base and key from initially generated values
  Serial.print("\nValid base/key index: ");
  for (int i = 0; i < messageLength; i++) {
    Serial.print(validIndex[i]);
    Serial.print(", ");
  }
  // print the valid bases
  Serial.print("\nValid bases: ");
  for (int i = 0; i < messageLength; i++) {
    Serial.print(validBases[i]);
    Serial.print(", ");
  }
  // print the key generated
  Serial.print("\nKey bits: ");
  for (int i = 0; i < messageLength; i++) {
    Serial.print(validKeys[i]);
    Serial.print(", ");
  }


  delay(signalPeriod); // delay before halting processor to complete any serial communications
  exit(0); // halts the processor preventing [void loop()] from repeating the experiment
}

// generate random bases and bits for key generation
//[+ basis: 0, 2], [x basis: 1, 3], [0 = 0 deg, 1 = 45 deg, 2 = 90 deg, 3 = -45 deg]
void genBases() {
  for (int i = 0; i < keyLength; i++) {
    aliceBasisList[i] = random(0, 4);
    bobBasisList[i] = random(0, 2);
    switch (aliceBasisList[i]) {
      case 0:
        aliceKeyBits[i] = 1;
        break;
      case 1:
        aliceKeyBits[i] = 1;
        break;
      case 2:
        aliceKeyBits[i] = 0;
        break;
      case 3:
        aliceKeyBits[i] = 0;
        break;
    }
  }
}

// generates bits based on random bases for both alice and bob and an initial random selection of bits by alice
//[+ basis: 0, 2], [x basis: 1, 3], [0 = 0 deg, 1 = 45 deg, 2 = 90 deg, 3 = -45 deg]
int basisTest() {
  int errors;
  for (int i = 0; i < keyLength; i++) {
    digitalWrite(laser, HIGH); // laser on
    state = digitalRead(sensor);
    // if laser is not detected assign a random value to the key list 
    // rework this? but if no value is assigned the array will have a null value causing problems. 
    // a random value could be considered the same as an error for the purposes of this experiment
    if (state == 0) {
      keyDetectionList[i] = random(0, 2); // random(low, high) low - inclusive, high - exclusive
      errors++;
    }
    // if alice polarized at 0 deg or 90 deg and bob polarized at 0 deg record the bit (should be 0)
    else if (state == 1 && (((aliceBasisList[i] == 0) || (aliceBasisList[i] == 2)) && (bobBasisList[i] == 0))) {
      keyDetectionList[i] = aliceKeyBits[i];
      laserDetected++;
    } 
    // if alice polarized at 45 or -45 deg and bob polarized at 45 deg record the bit (should be 1)
    else if (state == 1 && (((aliceBasisList[i] == 1) || (aliceBasisList[i] == 3)) && (bobBasisList[i] == 1))) {
      keyDetectionList[i] = aliceKeyBits[i];
      laserDetected++;
    } 
    // if the bases do not match assign a random bit (0 or 1) to the key list as there is 50% chance of error in a physical system
    else {
      keyDetectionList[i] = random(0, 2);
      laserDetected++;
      errors++;
    }
    delay(signalPeriod); // 1 second delay between laser pulses
    digitalWrite(laser, LOW); // laser off
    delay(signalPeriod);
  }
  return errors;
}

// assembling the key, same base checking logic as basisTest()
bool keyGen() {
  int keyCount = 0;
  for (int i = 0; i < keyLength; i++) {
    if (((aliceBasisList[i] == 0) || (aliceBasisList[i] == 2)) && (bobBasisList[i] == 0)) {
      validKeys[keyCount] = keyDetectionList[i];
      validBases[keyCount] = aliceBasisList[i];
      validIndex[keyCount] = i;
      keyCount++;
    } else if (((aliceBasisList[i] == 1) || (aliceBasisList[i] == 3)) && (bobBasisList[i] == 1)) {
      validKeys[keyCount] = keyDetectionList[i];
      validBases[keyCount] = aliceBasisList[i];
      validIndex[keyCount] = i;
      keyCount++;
    }
    if (keyCount == messageLength) {
      return true; // returns true when a number of bits with matching bases are found
    }
  }
  return false; // returns false when there are insufficient matching bits, new bases and bits need to be chosen using genBases() and basisTest()
}