#package bar
#import std
#import foo/cat

int DrinksNeeded(int drinksHadSoFar) {
  return drinksHadSoFar * foo::cat::Furryness();
}
