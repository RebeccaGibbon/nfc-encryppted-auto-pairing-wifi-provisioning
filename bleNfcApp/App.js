/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React, {
  useEffect,
  useState,
} from 'react';

import {
  Button,
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

// function to manage the items in the flatlist
const ListItem = ({ item, onPress, backgroundColor, textColor }) => (
  <TouchableOpacity onPress={onPress} style={[styles.item, backgroundColor]}>
    <Text style={[styles.nameText, textColor]}>Name: {item.name}</Text>
    <Text style={[styles.subText, textColor]}>MAC address or UUID: {item.id}</Text>
  </TouchableOpacity>
);

const App = () => {
  const [isScanning, setIsScanning] = useState(false);
  const [selectedId, setSelectedId] = useState(null);

  // function to start scanning for ble devices
  const startBleScan = () => {
    console.log("inside start ble function");
    console.log(isScanning);
    if(!isScanning){
      setIsScanning(true);
      console.log("requesting BLE permission");
      const permission = requestLocationPermission();
      if(permission){
        const subscription = manager.onStateChange((state) => {
          if (state == 'PoweredOn') {
            manager.startDeviceScan(null, null, (error, device) => {
              console.log("scanning...");
              console.log(device.name);
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
  };
  
  // function to switch the text and background colour of a listed item on press 
  const renderListItem = ({ item }) => {
    const backgroundColor = item.id === selectedId ? "peru" : "papayawhip";
    const color = item.id === selectedId ? 'white' : 'grey';

    return (
      <ListItem
        item={item}
        onPress={() => setSelectedId(item.id)}
        backgroundColor={{ backgroundColor }}
        textColor={{ color }}
      />
    );
  };

  return (
 
    <SafeAreaView style={styles.container}>
      <View style = {styles.headerStyle}>
        <Text style = {styles.titleText}>Wi-Fi Connect for ESP32</Text>
      </View>
      <TouchableOpacity
        style = {styles.button}
        onPress = {() => startBleScan()}
        >
        <Text style = {{color: "snow", fontSize: 18}}>
          {'Scan for BLE Devices:' + (isScanning ? 'on' : 'off')}
        </Text>
      </TouchableOpacity>
      <FlatList
        data={deviceList}
        renderItem={renderListItem}
        keyExtractor={(item) => item.id}
        extraData={selectedId}
      />
    </SafeAreaView>

    
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: "snow",
    alignItems: "center",
    // marginTop: StatusBar.currentHeight || 0,
  },
  headerStyle: {
    flex: 0.2,
    width: '100%',
    height: 45,
    justifyContent: "center",
    alignItems: "center",
    backgroundColor: "cadetblue",
  },
  button: {
    marginTop: 50,
    marginBottom: 20,
    paddingVertical: 10,
    paddingHorizontal: 30,
    borderRadius: 4,
    backgroundColor: "tan",
    alignItems: "center",
    minWidth: "48%",
  },
  item: {
    padding: 20,
    width: 275,
    marginVertical: 8,
    marginHorizontal: 16,
    borderRadius: 4,
  },
  nameText: {
    fontSize: 16,
  },
  subText: {
    fontSize: 14,
  },
  titleText: {
    fontSize: 28,
    color: "snow",
  },
});


export default App;