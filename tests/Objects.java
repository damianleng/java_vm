public class Objects {
    int x;
    int y;

    public Objects(int x, int y) {
        this.x = x;
        this.y = y;
    }

    public int sum() {
        return this.x + this.y;
    }

    public static int add(int a, int b) {
        return a + b;
    }

    public static void main(String[] args) {
        Objects obj = new Objects(3, 4);
        System.out.println(obj.sum());
        System.out.println(add(10, 20));
    }
}
