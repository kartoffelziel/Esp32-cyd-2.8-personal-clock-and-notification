# CYD Clock

Đồng hồ để bàn / trên xe cho board **ESP32-2432S028 (CYD 2.8")**. Viết lại từ đầu (Arduino + PlatformIO + TFT_eSPI),
số và chữ được vẽ ở đúng kích thước gốc nên nét sắc, không bị nhòe do phóng to.

## Có gì

**Màn hình chính:** đồng hồ `HH:MM` ở giữa, thứ / ngày / tháng bên dưới, nút cài đặt (góc trái) và nút nguồn (góc phải).
Chạm vào chỗ trống để ẩn các nút (chỉ còn đồng hồ), chạm lần nữa để hiện; có thể đặt tự ẩn sau N giây.

**Cài đặt** có 4 tab:

| Tab | Nội dung |
|---|---|
| Chung | độ sáng đèn nền, độ sáng nền, tự đổi màu nền theo bình minh / ban ngày / hoàng hôn / đêm (bật tắt được), màu nền cố định, chế độ nút nguồn, tự ẩn nút, xoay 180°, hiệu chỉnh cảm ứng, khôi phục gốc |
| Kết nối | quét WiFi, danh sách mạng, nhập mật khẩu bằng bàn phím trên màn hình, múi giờ GMT (bước 30 phút), tự kết nối khi bật, đồng bộ giờ NTP |
| Đồng hồ | kiểu số, cỡ số, chỉnh vị trí X/Y bằng mũi tên (xem trực tiếp trên đồng hồ thật), 12/24 giờ, số 0 đầu giờ, dấu `:` nhấp nháy, hiệu ứng (mờ dần khi đổi số / cầu vồng / thở), màu số, hiện ngày, cỡ chữ và kiểu chữ ngày |
| Thông báo | popup giữa màn hình, thẻ chạy dòng phía dưới, cỡ chữ, màu nền và màu chữ, hiệu ứng (trượt vào / nháy viền / nháy màn hình), thời gian hiện, nút **Xem trước** |

**Nút nguồn:** chạm để tắt màn hình (đồng hồ vẫn chạy, chạm màn hình để bật lại) hoặc chế độ **Ngủ** (tắt WiFi, tiết kiệm điện, chạm hoặc bấm BOOT để bật). Chọn chế độ ở tab Chung.

## Kiểu chữ

Kiểu số: Hiện đại (Outfit), JetBrains Mono, Hẹp (Big Shoulders), Công nghệ (Jura), Serif (Gloock), Thanh mảnh (Poiret One),
Pixel, **Arial (Liberation Sans)**, **Helvetica (TeX Gyre Heros)**, **Calibri (Carlito)**, **LED 7 đoạn**, **Digital-7 (nghiêng)**.

Kiểu chữ ngày: Sans (DejaVu), Arial (Liberation), Helvetica (TeX Gyre), Calibri (Carlito), JetBrains Mono, Serif (Lora).

Arial, Helvetica và Digital-7 là font có bản quyền hoặc chỉ miễn phí cho cá nhân nên **không được đóng gói**. Thay vào đó dùng bản
tương đương miễn phí: Liberation Sans (cùng kích thước ký tự với Arial), TeX Gyre Heros (bản làm lại của Helvetica), còn hai kiểu LED
được vẽ bằng code (không dùng file font Digital-7). Giấy phép các font nằm trong thư mục `licenses/`.
Fira Code chưa có trong bản này; muốn thêm, sửa `tools/bake_fonts.py` rồi chạy lại để tạo `src/fonts_data.cpp`.

## Build bằng GitHub (không cần cài gì)

1. Tạo repo, tải toàn bộ nội dung thư mục này lên (kéo thả trên máy tính). Thư mục `.github` bắt đầu bằng dấu chấm nên dễ bị sót:
   nếu không thấy nó trong repo, bấm **Add file → Create new file**, gõ tên `.github/workflows/build.yml` và dán nội dung file cùng tên.
2. Vào tab **Actions → Build firmware → Run workflow** (lần đầu có thể phải bấm nút bật workflow). Build mất vài phút.
3. Tải artifact `cyd-clock-firmware`, giải nén, lấy `cyd-clock-factory.bin`.
4. Nạp file đó ở offset **0x0** bằng trang flash như trước (chọn ESP32 CYD / Classic, 0x0 Factory). Nên chọn xóa flash.

Build tại máy: `pip install platformio` rồi `pio run -e cyd_inv`. Môi trường `cyd_inv` là ILI9341 có đảo màu, đúng cấu hình cho ra màu chuẩn trên board của bạn.

## Lần đầu bật máy

Màn hình hiện **hiệu chỉnh cảm ứng**: chạm đúng giữa dấu `+` ở 4 góc, rồi chạm thử và bấm **Lưu**. Cách này tự dò hướng của tấm cảm ứng
nên không cần biết board đặt xoay thế nào. Dòng `raw x, y` ở cuối màn hình hiện số đọc thô, nếu chạm mà số không đổi thì cảm ứng chưa được đọc.
Làm lại bất cứ lúc nào ở Cài đặt → Chung → Hiệu chỉnh cảm ứng.

## Giờ lấy từ đâu

Board không có pin giữ giờ, cắt nguồn là mất giờ. Hiện tại giờ lấy từ WiFi (NTP), nên lưu WiFi một lần ở tab Kết nối là mỗi lần bật máy tự có giờ.
Khi chưa có giờ, màn hình hiện `- - : - -`. Lấy giờ và thông báo từ điện thoại qua Bluetooth là giai đoạn kế tiếp (xem bên dưới).

## Chưa có trong bản này

- Bluetooth: lấy giờ từ điện thoại, nhận thông báo từ điện thoại (cần thêm app Android). Giao diện thông báo và nút Xem trước đã sẵn.
- Tắt hẳn (deep sleep): bản này chỉ tắt màn hình hoặc ngủ nhẹ; muốn tắt hẳn thì rút nguồn.
- Nhiều hiệu ứng nền hơn.

## Thử giao diện trên máy tính (dành cho người sửa code)

`host/build_test.sh` build bộ mô phỏng giao diện (cùng code với firmware, chỉ thay phần phần cứng) và chạy các bài kiểm thử cảm ứng, cài đặt,
bàn phím, hiệu chỉnh, nút nguồn, thông báo. `host/render.py` đổi ảnh dump thành PNG.

## Chưa kiểm chứng trên board thật

Toàn bộ logic và hình ảnh đã được kiểm tra bằng bộ mô phỏng trên máy tính, và phần code board được kiểm tra cú pháp. Chưa được biên dịch bằng
toolchain ESP32 và chưa chạy trên phần cứng, nên lần build đầu có thể còn lỗi nhỏ. Hãy gửi log lỗi hoặc ảnh chụp màn hình để chỉnh tiếp.
