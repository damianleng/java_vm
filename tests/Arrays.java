public class Arrays {
    public static void main(String[] args) {
        int[] arr = new int[5];
        for (int i = 0; i < 5; i++) {
            arr[i] = i * 2;
        }
        System.out.println(arr[3]);      // 6
        System.out.println(arr.length);  // 5
    }
}
