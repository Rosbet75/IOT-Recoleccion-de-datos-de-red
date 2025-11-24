<?php
$conn = new mysqli("mysql", "iotuser", "iotpass", "iot");
if ($conn->connect_error) { 
    die("Error de conexión"); 
}
$limit = isset($_GET['limit']) ? intval($_GET['limit']) : 1000;

$sql = "SELECT * FROM sniff_data ORDER BY id DESC LIMIT 1000";
$result = $conn -> query($sql);

$data =[];
if ($result -> num_rows > 0){
    while($row = $result -> fetch_assoc()){
        $data[] = $row;
    }
}
header('Content-Type: application/json');
echo json_encode($data);
$conn->close();
?>