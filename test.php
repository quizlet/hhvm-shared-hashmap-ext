<?php
var_dump(extension_loaded("shared_hashmap"));
var_dump(function_exists("shhashmap_init"));
var_dump(function_exists("shhashmap_size"));
var_dump(function_exists("shhashmap_add"));
var_dump(function_exists("shhashmap_get"));
var_dump(shhashmap_init('test'));
var_dump(shhashmap_add('test', "abc", "123"));
var_dump(shhashmap_add('test', "def", "456"));
var_dump(shhashmap_get('test', "abc"));
var_dump(shhashmap_get('test', "def"));
var_dump(shhashmap_delete('test', "def"));
var_dump(shhashmap_get('test', "def"));

$sharedHashMap = new SharedHashMap('test2');
var_dump($sharedHashMap);
var_dump($sharedHashMap->add("ghi", "789"));
var_dump($sharedHashMap->add("jkl", "101112"));
var_dump($sharedHashMap->get("ghi"));
var_dump($sharedHashMap->get("jkl"));
var_dump($sharedHashMap->delete("ghi"));
var_dump($sharedHashMap->get("ghi"));