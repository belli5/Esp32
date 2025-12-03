import styled from "styled-components";

export const PageWrapper = styled.div`
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #f3f4f6;
  padding: 24px;
`;

export const Card = styled.div`
  width: 100%;
  max-width: 900px;
  background: #ffffff;
  border-radius: 24px;
  padding: 32px 32px 40px;
  box-shadow: 0 18px 45px rgba(15, 23, 42, 0.12);
`;

/* -------- Tabs iguais das outras telas (3 abas) -------- */

export const TabsBar = styled.div`
  margin-bottom: 24px;
  display: flex;
  justify-content: center;
`;

export const TabsTrack = styled.div`
  position: relative;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  width: 360px;
  padding: 4px;
  background: #e5e7eb;
  border-radius: 999px;
`;

export const Slider = styled.div`
  position: absolute;
  top: 4px;
  left: 4px;
  width: calc((100% - 8px) / 3);
  height: calc(100% - 8px);
  border-radius: 999px;
  background: #0b5cff;
  transform: translateX(${({ activeIndex }) => activeIndex * 100}%);
  transition: transform 0.25s ease;
  z-index: 0;
`;

export const TabButton = styled.button`
  position: relative;
  z-index: 1;
  border: none;
  background: transparent;
  cursor: pointer;
  font-size: 0.9rem;
  font-weight: 600;
  padding: 8px 0;
  border-radius: 999px;
  color: ${({ active }) => (active ? "#ffffff" : "#4b5563")};
  transition: color 0.2s ease;
  font-family: inherit;
`;

/* -------- Títulos -------- */

export const Title = styled.h1`
  font-size: 32px;
  font-weight: 800;
  color: #020617;
  margin: 0 0 4px;
  text-align: center;
`;

export const Subtitle = styled.p`
  font-size: 16px;
  color: #6b7280;
  margin: 0 0 24px;
  text-align: center;
`;

/* -------- Grade dos gráficos -------- */

export const ChartsGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
  margin-bottom: 24px;

  @media (max-width: 900px) {
    grid-template-columns: 1fr;
  }
`;

export const ChartCard = styled.div`
  border-radius: 18px;
  padding: 5px 10px 5px;
  background: #f9fafb;
  border: 1px solid #e5e7eb;
  box-shadow: 0 4px 12px rgba(15, 23, 42, 0.05);
`;

export const ChartTitle = styled.h2`
  font-size: 16px;
  margin: 10px;
  color: #111827;
`;

export const ChartSubtitle = styled.p`
  font-size: 12px;
  margin: 16px 16px 16px;
  color: #6b7280;
`;

export const ChartArea = styled.div`
  padding-top: 10px;
`;

export const BarRow = styled.div`
  display: flex;
  align-items: flex-end;
  gap: 8px;
  height: 90px;
  margin-bottom: 8px;
`;

export const Bar = styled.div`
  flex: 1;
  border-radius: 999px 999px 4px 4px;
  /* só pinta quando value > 0 */
  background: ${({ value }) =>
    value <= 0 ? "transparent" : "linear-gradient(180deg, #2563eb, #1d4ed8)"};
  /* se value <= 0, altura zero */
  height: ${({ value }) =>
    value <= 0 ? "0%" : `${Math.min(value, 100)}%`};
  box-shadow: ${({ value }) =>
    value <= 0 ? "none" : "0 4px 10px rgba(37, 99, 235, 0.35)"};
  position: relative;
  cursor: ${({ value }) => (value <= 0 ? "default" : "pointer")};
  transition: transform 0.18s ease;

  &:hover {
    transform: ${({ value }) => (value <= 0 ? "none" : "translateY(-4px)")};
  }

  /* Tooltip com o número real (usa data-count do JSX) */
  &::after {
    content: attr(data-count);
    position: absolute;
    bottom: 100%;
    left: 50%;
    transform: translateX(-50%);
    padding: 4px 8px;
    border-radius: 999px;
    background: #111827;
    color: #f9fafb;
    font-size: 11px;
    white-space: nowrap;
    opacity: 0;
    pointer-events: none;
    transition: opacity 0.15s ease, bottom 0.15s ease;
  }

  &:hover::after {
    opacity: 1;
    bottom: calc(100% + 6px);
  }
`;



export const BarLabelRow = styled.div`
  display: flex;
  justify-content: space-between;
  gap: 8px;
`;

export const BarLabel = styled.span`
  flex: 1;
  text-align: center;
  font-size: 11px;
  color: #6b7280;
`;

/* -------- Resumo da semana -------- */

export const WeekSummaryCard = styled.div`
  margin-top: 8px;
  border-radius: 18px;
  padding: 16px 18px 14px;
  background: #f9fafb;
  border: 1px solid #e5e7eb;
`;

export const WeekSummaryTitle = styled.h3`
  margin: 0 0 10px;
  font-size: 15px;
  color: #111827;
`;

export const WeekDaysRow = styled.div`
  display: flex;
  gap: 8px;
  margin-bottom: 10px;
  flex-wrap: wrap;
`;

export const DayPill = styled.span`
  padding: 6px 10px;
  border-radius: 999px;
  font-size: 12px;
  font-weight: 500;
  color: ${({ presente }) => (presente ? "#166534" : "#991b1b")};
  background: ${({ presente }) => (presente ? "#bbf7d0" : "#fee2e2")};
`;

export const LegendRow = styled.div`
  display: flex;
  gap: 16px;
  flex-wrap: wrap;
  font-size: 12px;
  color: #4b5563;
`;

export const LegendItem = styled.div`
  display: inline-flex;
  align-items: center;
  gap: 6px;
`;

export const LegendDot = styled.span`
  width: 10px;
  height: 10px;
  border-radius: 999px;
  background: ${({ type }) =>
    type === "presente" ? "#22c55e" : "#ef4444"};
`;

export const LogList = styled.div`
  max-height: 190px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding-right: 4px;
`;

export const LogRow = styled.div`
  padding: 8px 10px;
  border-radius: 10px;
  background: #ffffff;
  border: 1px solid #e5e7eb;
  display: flex;
  flex-direction: column;
  gap: 4px;
  font-size: 12px;
`;

export const LogMain = styled.div`
  display: flex;
  align-items: center;
  gap: 4px;
  color: #111827;
`;

export const LogStrong = styled.span`
  font-weight: 600;
  font-family: "JetBrains Mono", ui-monospace, SFMono-Regular, Menlo, Monaco,
    Consolas, "Liberation Mono", "Courier New", monospace;
  font-size: 11px;
  background: #eef2ff;
  padding: 2px 6px;
  border-radius: 6px;
`;

export const LogMeta = styled.div`
  display: flex;
  align-items: center;
  gap: 6px;
  color: #6b7280;
  font-size: 11px;
`;

export const EmptyLog = styled.div`
  font-size: 12px;
  color: #9ca3af;
  text-align: center;
  padding: 16px 0;
`;
