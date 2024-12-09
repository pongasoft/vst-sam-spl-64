#include "version.h"

// Check https://developer.apple.com/library/archive/documentation/General/Conceptual/ExtensibilityPG/AudioUnit.html for various types

/* Bundle Identifier */
#define kAudioUnitBundleIdentifier	pongasoft.vst3.SamSpl64.audiounit

/* Version Number (computed in version.h from version in CMakeLists.txt) */
#define kAudioUnitVersion			AU_VERSION_INT

/* Company Name + Effect Name */
#define kAUPluginName 				pongasoft: Sam-Spl 64

/* A product name for the audio unit, such as TremoloUnit */
#define kAUPluginDescription 		Sam-Spl 64

/*
  The specific variant of the Audio Unit. The four possible types and their values are:
  Effect (aufx), Generator (augn), Instrument (aumu), and Music Effect (aufm).
 */
#define kAUPluginType 				aumu

/* A subtype code for the audio unit, such as tmlo. This value must be exactly 4 alphanumeric characters. */
#define kAUPluginSubType 			unkw

/* A manufacturer code for the audio unit, such as Aaud. This value must be exactly 4 alphanumeric characters.
 * This value should be unique across audio units of the same manufacturer
 * Manufacturer OSType should have at least one non-lower case character */
#define kAUPluginManufacturer 		Psft

// Definitions for the resource file
#define kAudioUnitName				      "pongasoft: Sam-Spl 64" // same as kAUPluginName
#define kAudioUnitDescription	      "Sam-Spl 64" // same as kAUPluginDescription
#define kAudioUnitType				      'aumu' // same as kAUPluginType
#define kAudioUnitComponentSubType	'unkw' // same as kAUPluginSubType
#define kAudioUnitComponentManuf    'Psft' // same as kAUPluginManufacturer

#define kAudioUnitCarbonView		1		// if 0 no Carbon view support will be added
