import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.VarHandle;

public class nr_cas {
    public int x = 2;

    public void doCas() {
        MethodHandles.Lookup l = MethodHandles.lookup();

        try {
            VarHandle vh = l.findVarHandle(nr_cas.class, "x", int.class);
            vh.compareAndSet(this, 2, 6);
        } catch (NoSuchFieldException | IllegalAccessException e) {
            System.out.println("Exception");
        }
    }

    public static void main(String[] args) {
        nr_cas vh = new nr_cas();
    
        Thread t1 = new Thread(() -> {
            vh.doCas();
        });
        Thread t2 = new Thread(() -> {
            vh.doCas();
        });
        t1.start();
        t2.start();
        try {
            t1.join();
            t2.join();
        } catch (InterruptedException e) {
            System.out.println("Interrupted");
        }
        if (vh.x != 2) {
            System.out.println("x = " + vh.x);
        }
    }
}
