public class nr_before_start {
    public static int x;
    public static int y;

    public static void main(String... args) {
	x = 12;
        Thread t1 = new Thread(() -> {
            increment1();
        });

	y = 13;
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

        x = 3;
        y = 4;
    }

    public static void increment1() {
        x++;
    }

    public static void increment2() {
        y++;
    }
}
