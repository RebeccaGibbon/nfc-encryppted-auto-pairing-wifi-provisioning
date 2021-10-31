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
  FlatList,
  PermissionsAndroid,
  SafeAreaView,
  ScrollView,
  StatusBar,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from 'react-native';
import { BleManager } from 'react-native-ble-plx';
export const manager = new BleManager();


// Request location permission from android device (needed for BT)
const requestLocationPermission = async() => {
  
    const granted = await PermissionsAndroid.request(PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION); 
    if (granted === PermissionsAndroid.RESULTS.GRANTED) {
      console.log('Location permission for bluetooth scanning granted');
      return true;
    } else {
      console.log('Location permission for bluetooth scanning revoked');
      return false;
  }
}

// Format the devices in the flatlist
const ListItem = ({ item, onPress, backgroundColor, textColor }) => (
  <TouchableOpacity onPress={onPress} style={[styles.item, backgroundColor]}>
    <Text style={[styles.nameText, textColor]}>Name: {item.name}</Text>
    <Text style={[styles.subText, textColor]}>MAC address/UUID: {item.id}</Text>
  </TouchableOpacity>
);

const App = () => {
  const [deviceCount, setDeviceCount] = useState(0);
  const [scannedDevices, setScannedDevices] = useState({});
  const [logData, setLogData] = useState([]);
  const [logCount, setLogCount] = useState(0);

  useEffect(() => {
    manager.onStateChange((state) => {
      const subscription = manager.onStateChange(async (state) => {
        console.log(state);
        const newLogData = logData;
        newLogData.push(state);
        await setLogCount(newLogData.length);
        await setLogData(newLogData);
        subscription.remove();
      }, true);
      return () => subscription.remove();
    });
  }, [manager]);

  // Turn on BT (if off) and scan for BLE devices
  const startBleScan = async() => {
    console.log("inside start ble function");

    const btState = await manager.state();
    if(btState !== 'PoweredOn'){
      await manager.enable();
    }
    console.log("requesting BLE permission");
    const permission = await requestLocationPermission();
    if (permission) {
      manager.startDeviceScan(null, null, async (error, device) => {
        console.log("Scanning...");
        if (error) {
          console.log(error);
          return (false);
        }
        if (device) {
          // console.log("Device name:" + device.name);
          // console.log("Device id:" + device.id);
          const newScannedDevices = scannedDevices;
          newScannedDevices[device.id] = device;
          await setDeviceCount(Object.keys(newScannedDevices).length);
          await setScannedDevices(scannedDevices);
        }
      });
    }
    return (true);
  };

  // Connect to selected BLE device
  const connectToBleDevice = async(device) => {
    console.log("connecting to: " + device.name);
    const connect = manager.connectToDevice(device.id, {autoConnect: false});
    if(connect){
      console.log("Connection established...");
    }
    return true;
  }
  
  // Switch the text and background colour of a listed item on press 
  const renderListItem = ({ item }) => {
    const backgroundColor = item.id === scannedDevices.name ? "peru" : "papayawhip";
    const color = item.id === scannedDevices.name ? 'white' : 'grey';
    manager.stopDeviceScan();

    return (
      <ListItem
        item={item}
        onPress={() => connectToBleDevice(item)}
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
          {'Scan for BLE Devices'}
        </Text>
      </TouchableOpacity>
      <FlatList
        data={Object.values(scannedDevices)}
        renderItem={renderListItem}
      />
    </SafeAreaView>  
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: "snow",
    alignItems: "center",
  },
  headerStyle: {
    flex: 0.15,
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
    // width: 275,
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