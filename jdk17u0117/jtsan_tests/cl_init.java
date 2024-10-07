class baz {
    public static int foo;

    static {
        foo = 42;
    }

    public static int getFoo() {
        return foo;
    }
}

class bar {
    public static int foo;

    static {
        foo = baz.getFoo();
    }
}


public class cl_init {
    public static void main(String[] args) {
        Thread t1 = new Thread(() -> {
            bar a = new bar();
            System.out.println(baz.foo);
        });

        Thread t2 = new Thread(() -> {
            baz a = new baz();
            System.out.println(bar.foo);
        });

        t1.start();
        t2.start();

        try {
            t1.join();
            t2.join();
        } catch (InterruptedException e) {
            System.out.println("Interrupted");
        }
    }
}
