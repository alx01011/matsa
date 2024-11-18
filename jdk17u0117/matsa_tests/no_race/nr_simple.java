public class nr_simple {
    public static int x = 3;
    public static int y = 4;

    public static void main(String... args) {
        Thread t1 = new Thread(() -> {
            increment1();
        });

        Thread t2 = new Thread(() -> {
            increment2();
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

    public static void increment1() {
        x++;
    }

    public static void increment2() {
        y++;
    }
}