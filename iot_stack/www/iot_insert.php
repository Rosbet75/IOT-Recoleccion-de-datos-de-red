<?php
$conn = new mysqli("mysql", "iotuser", "iotpass", "iot");
if ($conn->connect_error) { die("Error de conexión"); }

$rssi = $_POST['rssi'];
$channel = $_POST['channel'];
$mac_src = $_POST['mac_src'];
$mac_dst = $_POST['mac_dst'];
$mac_bssid = $_POST['mac_bssid'];
$tipo = $_POST['tipo'];
$subtipo = $_POST['subtipo'];
$temp = $_POST['temp'];
$hum = $_POST['hum'];
$dist = $_POST['distancia'];
$touch4 = $_POST['touch4'];
$touch5 = $_POST['touch5'];

$sql = "INSERT INTO sniff_data
        (rssi, channel, mac_src, mac_dst, mac_bssid, tipo, subtipo, temp, hum, distancia, touch4, touch5)
        VALUES
        ('$rssi', '$channel', '$mac_src', '$mac_dst', '$mac_bssid', '$tipo', '$subtipo', '$temp', '$hum', '$dist', '$touch4', '$touch5')";

if ($conn->query($sql) === TRUE) {
    echo "OK";
} else {
    echo "ERR";
}

$conn->close();
?>
