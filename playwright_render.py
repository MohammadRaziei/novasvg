import argparse
import asyncio
from pathlib import Path
from playwright.async_api import Page, async_playwright


def parse_cli_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render SVG to PNG using Playwright Chromium"
    )
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
        default=None,
        help="Output PNG file path (default: <input_filename>.png in CWD)",
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
        default=None,
        help="Viewport width (default: auto-detected from SVG)",
    )
    parser.add_argument(
        "--height",
        "-H",
        type=int,
        default=None,
        help="Viewport height (default: auto-detected from SVG)",
    )
    parser.add_argument(
        "--scale",
        "-s",
        type=int,
        default=2,
        help="Device scale factor (default: 2)",
    )
    return parser.parse_args()


async def get_svg_dimensions(page: Page) -> dict[str, int]:
    return await page.evaluate(
        """() => {
        const svg = document.querySelector('svg');
        if (!svg) return { width: 800, height: 600 };
        const rect = svg.getBoundingClientRect();
        return {
            width: Math.ceil(rect.width) || 800,
            height: Math.ceil(rect.height) || 600
        };
    }"""
    )


async def render_svg_to_png(
    input_path: Path,
    output_path: Path,
    width: int | None = None,
    height: int | None = None,
    scale: int = 2,
    bg_color: str = "transparent",
):
    async with async_playwright() as p:
        browser = await p.chromium.launch()
        page = await browser.new_page(device_scale_factor=scale)
        await page.goto(input_path.as_uri())

        if width is None or height is None:
            detected = await get_svg_dimensions(page)
            width = width if width is not None else detected["width"]
            height = height if height is not None else detected["height"]

        await page.set_viewport_size({"width": width, "height": height})

        is_transparent = bg_color.lower() == "transparent"
        if not is_transparent and bg_color.lower() != "white":
            await page.add_style_tag(
                content=f"body {{ background-color: {bg_color} !important; }}"
            )

        await page.screenshot(path=str(output_path), omit_background=is_transparent)
        await browser.close()

        calculated_w = width * scale
        calculated_h = height * scale
        print(
            f"Done -> Saved to {output_path.name} ({calculated_w}x{calculated_h}px, BG: {bg_color})"
        )


async def main():
    args = parse_cli_args()

    svg_path = Path(args.input).resolve()
    if not svg_path.exists():
        raise FileNotFoundError(f"Input file not found: {svg_path}")

    output_path = (
        Path.cwd() / f"{svg_path.stem}.png"
        if args.output is None
        else Path(args.output).resolve()
    )

    await render_svg_to_png(
        input_path=svg_path,
        output_path=output_path,
        width=args.width,
        height=args.height,
        scale=args.scale,
        bg_color=args.bg_color,
    )


if __name__ == "__main__":
    asyncio.run(main())