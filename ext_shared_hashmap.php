<?hh

<<__Native>>
function shhashmap_init(string $map_name): bool;

<<__Native>>
function shhashmap_size(string $map_name): int;

<<__Native>>
function shhashmap_add(string $map_name, string $key, string $value): bool;

<<__Native>>
function shhashmap_get(string $map_name, string $key): ?string;

class SharedHashMap {

	/*
	 * This will create a shared hash map if it does not already exist, accessible between threads.
	 * If it does exist then it won't do anything. This is a fast operation.
	 */
	public function __construct(private string $name) {
		shhashmap_init($name);
	}

	// This will return the current size of the hash map
	public function size(): int {
		return shhashmap_size($this->name);
	}

	// This will add a row into the hash map with the given value
	public function add(string $key, string $value): bool {
		return shhashmap_add($this->name, $key, $value);
	}

	// This will return the value at the given key
	public function get(string $key): ?string {
		return shhashmap_get($this->name, $key);
	}
}