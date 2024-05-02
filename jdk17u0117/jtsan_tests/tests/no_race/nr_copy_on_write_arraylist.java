public class nr_copy_on_write_arraylist {
    public static void main(String... args) {
        java.util.concurrent.CopyOnWriteArrayList<Integer> list = new java.util.concurrent.CopyOnWriteArrayList<>();
        list.add(1);
        list.add(2);

        Thread t1 = new Thread(() -> {
            increment(list, 1);
        });

        Thread t2 = new Thread(() -> {
            increment(list, 2);
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

    public static void increment(java.util.concurrent.CopyOnWriteArrayList<Integer> list, int index) {
        list.set(index, list.get(index) + 1);
    }
}