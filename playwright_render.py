import argparse
import asyncio
from pathlib import Path
from playwright.async_api import async_playwright


async def main():
    parser = argparse.ArgumentParser(
        description="Render SVG to PNG using Playwright Chromium"
    )

    # اجباری کردن ورودی (required=True)
    parser.add_argument(
        "--input",
        "-i",
        type=str,
        required=True,
        help="Path to the input SVG file (Required)",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=str,
        default="chrome-reference.png",
        help="Output PNG file path (default: chrome-reference.png)",
    )
    parser.add_argument(
        "--bg-color",
        "-bg",
        type=str,
        default="transparent",
        help="Background color: 'transparent', 'white', or any CSS color (default: transparent)",
    )
    parser.add_argument(
        "--width",
        "-w",
        type=int,
        default=441,
        help="Viewport width (default: 441)",
    )
    parser.add_argument(
        "--height",
        "-H",
        type=int,
        default=517,
        help="Viewport height (default: 517)",
    )
    parser.add_argument(
        "--scale",
        "-s",
        type=int,
        default=2,
        help="Device scale factor (default: 2)",
    )

    args = parser.parse_args()

    svg_path = Path(args.input).resolve()
    if not svg_path.exists():
        raise FileNotFoundError(f"Input file not found: {svg_path}")

    async with async_playwright() as p:
        browser = await p.chromium.launch()
        page = await browser.new_page(
            viewport={"width": args.width, "height": args.height},
            device_scale_factor=args.scale,
        )

        file_url = svg_path.as_uri()
        await page.goto(file_url)

        is_transparent = args.bg_color.lower() == "transparent"

        if not is_transparent and args.bg_color.lower() != "white":
            await page.add_style_tag(
                content=f"body {{ background-color: {args.bg_color} !important; }}"
            )

        await page.screenshot(path=args.output, omit_background=is_transparent)

        await browser.close()

        calculated_width = args.width * args.scale
        calculated_height = args.height * args.scale
        print(
            f"Done -> Saved to {args.output} ({calculated_width}x{calculated_height}px, BG: {args.bg_color})"
        )


if __name__ == "__main__":
    asyncio.run(main())