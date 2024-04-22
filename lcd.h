#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address // In my case 0x27

//uint8_t bell[8]  = {0x4, 0xe, 0xe, 0xe, 0x1f, 0x0, 0x4};
//uint8_t clock[8] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0};

unsigned long lastLcdUpdate = 0;


//setup
void lcd_setup() {
  // int lcd
  lcd.begin(40,4);
  lcd.backlight();

} //end lcd_setup()


// SHOW TIME ON LCD
void show_time_lcd(DateTime show) {
  // set time params
  get_time();
  // set output
  myDate = String(daysOfTheWeek[show.dayOfTheWeek()]) + " ";
  if (show.day() < 10)
    myDate += "0";
  myDate = myDate + show.day() + "/";
  if (show.month() < 10)
    myDate += "0";
  myDate = myDate + show.month() + "/" + show.year() ;

  byte tempHour = show.hour();
  if (tempHour > 12)
    tempHour = tempHour - 12;
  if (tempHour == 0)
    tempHour = 12;
  if (tempHour < 10)
    myTime = "0";
  else
    myTime = "";

  myTime = myTime + tempHour + ":";
  if (show.minute() < 10)
    myTime += "0";
  myTime = myTime + show.minute() + ":";
  if (show.second() < 10)
    myTime += "0";
  myTime = myTime + show.second() ;
  if (show.hour() > 11)
    myTime = myTime + " PM";
  else
    myTime = myTime + " AM";

  //Print on lcd
  lcd.setCursor(0, 0);
  lcd.print(myDate);
  lcd.setCursor(0, 1);
  lcd.print(myTime);

  // set last lcd update
  lastLcdUpdate = millis();
} // end show_time_lcd()

void lcd_display() {
  lcd.setCursor(0, 2);
  lcd.write(2);//battery logo
  lcd.setCursor(2, 2);
  lcd.print("12");
  lcd.print("V");
  lcd.setCursor(0, 3);
  lcd.print("Status:");
}
