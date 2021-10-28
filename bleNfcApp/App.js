/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React, {
  useEffect,
} from 'react';

import {
  FlatList,
  PermissionsAndroid,
  SafeAreaView,
  ScrollView,
  StatusBar,
  StyleSheet,
  Text,
  TouchableOpacity,
  useColorScheme,
  View,
} from 'react-native';

import { BleManager } from 'react-native-ble-plx';

export const manager = new BleManager();

//variables - test version of a BLE device list
const deviceList = [
  {id:'MAC address on Android and UUID on iOS', name: 'No devices detected'},
  {id:'this should be unique', name: 'Device name if present'},
];

// function to request ble permission from android device
async function requestLocationPermission() {
  try {
    const granted = await PermissionsAndroid.request(
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION, {
        title: 'Location permission for bluetooth scanning',
        message: 'Wi-Fi Connect for ESP32 needs access to your device location',
        buttonNeutral: 'Ask Me Later',
        buttonNegative: 'Cancel',
        buttonPositive: 'OK',
      },
    ); 
    if (granted === PermissionsAndroid.RESULTS.GRANTED) {
      console.log('Location permission for bluetooth scanning granted');
      return true;
    } else {
      console.log('Location permission for bluetooth scanning revoked');
      return false;
    }
  } catch (err) {
    console.warn(err);
    return false;
  }
}

// function to manage list items for BLE devices
const listItem = (deviceInfo) => {
  <View style = {styles.box}>
    <Text style = {styles.text}>
      {deviceInfo.name}
    </Text>
    <Text style = {styles.subText}>
      {deviceInfo.id}
    </Text>
  </View>
}

const App = () => {

  // function to manage ble scanning
  const startBle = () => {
    console.log("inside start ble function");
    const permission = requestLocationPermission();
    if(permission == PermissionsAndroid.RESULTS.GRANTED){
      const subscription = manager.onStateChange((state) => {
        if (state === 'PoweredOn') {
            manager.startDeviceScan(null, null, (error, device) => {
              console.log("scanning...");
              console.log(device.id);
              if(null){
                console.log("no devices found");
              }
              if (error) {
                console.log(error);
                return;
              }
            });
            subscription.remove();
        }
      }, true);
    }
  }

  const connectDevice = (deviceId) => {
    console.log("inside connect device function");
    manager.connectToDevice(deviceId, {autoconnect: true});
    console.log("connecting to device...");
    console.log(manager.isDeviceConnected(deviceId));
  }

  const renderItem = ({item}) => {
    console.log("inside render item");
    <listItem deviceInfo = {item}></listItem>
  }

  return (
    <SafeAreaView style = {styles.sectionTitleContainer}>
      <Text style = {styles.sectionTitle}>
        Wi-Fi Connect for ESP32
      </Text>
      <View style = {styles.sectionContainer}>
        <TouchableOpacity
        // include function call here to scan for BT devices
          onPress = {startBle()}
          style = {styles.button}
          >
          <Text style = {styles.buttonText}>
            Scan for Bluetooth Devices
          </Text>
          </TouchableOpacity>
          <View style = {styles.box}>
            <FlatList
              data = {deviceList}
              renderItem = {renderItem}
              keyExtractor = {(item) => item.id}
              
            >
              
            </FlatList>
          </View>
      </View>
    </SafeAreaView>
  )
};

const styles = StyleSheet.create({
  sectionTitleContainer: {
    flex: 1,
    paddingVertical: 5,
    backgroundColor: "cadetblue",
  },
  sectionContainer: {
    flex: 3,
    backgroundColor: "snow",
    alignItems: "center",
    padding: 10,
  },
  sectionTitle: {
    color: "snow",
    fontSize: 28,
    textAlign: 'center',
    fontWeight: '400',
  },
  buttonText: {
    color: "snow",
    fontSize: 18,
    fontWeight: '400',
  },
  highlight: {
    fontWeight: '700',
  },
  button: {
    marginTop: 50,
    paddingVertical: 10,
    paddingHorizontal: 30,
    borderRadius: 4,
    backgroundColor: "tan",
    alignItems: "center",
    minWidth: "48%",
  },
  box: {
    flex: 1,
    marginTop: StatusBar.currentHeight || 0,
    // marginTop: 20,
    // paddingVertical: 10,
    // paddingHorizontal: 30,
    width: 275,
    // height: 200,
    // alignItems: "center",
    backgroundColor: "seashell",
    borderRadius: 4,
  },
  item: {
    padding: 20,
    marginVertical: 8,
    marginHorizontal: 16,
  },
  text: {
    fontSize: 18,
  },
  subText: {
    fontSize: 12,
  },
});

export default App;