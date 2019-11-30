#include "FS.h"
 
void setup() {
  Serial.begin(115200);
  
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  File file = SPIFFS.open("/test.txt", "r");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  
  Serial.println();
  Serial.println("File Content:");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}
 
void loop() {

}


/*   для записи в файл
File file = SPIFFS.open("/test.txt", "w");
  if (!file) {
      Serial.println("file open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");

file.println("Fizz");
file.print("Buzz");

*/
