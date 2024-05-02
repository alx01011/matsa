public class nr_concurrent_hash_map {
    public static void main(String... args) {
        java.util.concurrent.ConcurrentHashMap<Integer, Integer> map = new java.util.concurrent.ConcurrentHashMap<>();
        map.put(1, 1);
        map.put(2, 2);

        Thread t1 = new Thread(() -> {
            increment(map, 1);
        });

        Thread t2 = new Thread(() -> {
            increment(map, 2);
        });

        t1.start();
        t2.start();

        try {
            t1.join();
            t2.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }

    public static void increment(java.util.concurrent.ConcurrentHashMap<Integer, Integer> map, int key) {
        map.put(key, map.get(key) + 1);
    }
}