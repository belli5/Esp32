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
  padding: 32px 32px 40px; /* IGUAL AO CADASTRO */
  box-shadow: 0 18px 45px rgba(15, 23, 42, 0.12);
  text-align: center;
`;

/* --------- Tabs iguais da Home --------- */

export const TabsBar = styled.div`
  margin-bottom: 24px;
  display: flex;
  justify-content: center;
`;

export const TabsTrack = styled.div`
  position: relative;
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  width: 260px;
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

  /* 0, 100%, 200% – anda “uma largura” por índice */
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

/* --------- Conteúdo --------- */

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
  margin: 0 0 32px;
  text-align: center;
`;

export const OptionsRow = styled.div`
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
  margin-bottom: 32px;

  @media (max-width: 900px) {
    grid-template-columns: 1fr;
  }
`;

export const OptionButton = styled.button`
  border: 2px solid
    ${({ active }) => (active ? "#3b82f6" : "transparent")};
  border-radius: 18px;
  padding: 20px 18px;
  background: ${({ active }) => (active ? "#eef6ff" : "#f9fafb")};
  text-align: left;
  cursor: pointer;
  font-family: inherit;
  transition: transform 0.12s ease, box-shadow 0.12s ease,
    background 0.12s ease, border-color 0.12s ease;
  box-shadow: ${({ active }) =>
    active
      ? "0 6px 18px rgba(59, 130, 246, 0.25)"
      : "0 4px 12px rgba(15, 23, 42, 0.05)"};

  &:hover {
    background: ${({ active }) =>
      active ? "#e0efff" : "#eef2ff"};
    transform: translateY(-1px);
    box-shadow: ${({ active }) =>
      active
        ? "0 8px 22px rgba(59, 130, 246, 0.35)"
        : "0 8px 20px rgba(15, 23, 42, 0.08)"};
  }

  &:active {
    transform: translateY(0);
  }
`;


export const OptionTitle = styled.h2`
  font-size: 18px;
  margin: 0 0 6px;
  color: #111827;
`;

export const OptionDescription = styled.p`
  font-size: 14px;
  margin: 0;
  color: #6b7280;
`;

/* --------- Área da LED --------- */

export const LedArea = styled.div`
  border-radius: 18px;
  background: #f9fafb;
  padding: 16px 20px;
  border: 1px solid #e5e7eb;
`;

export const LedRow = styled.div`
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
`;

export const LedDot = styled.div`
  width: 18px;
  height: 18px;
  border-radius: 999px;
  background: ${({ status }) => {
  if (status === "success") return "green";
  if (status === "error") return "red";
  if (status === "waiting" || status === "exists") return "yellow";
  return "gray";
}};
  box-shadow: ${({ status }) =>
    status === "idle"
      ? "none"
      : "0 0 12px rgba(0,0,0,0.15)"};
`;

export const LedText = styled.p`
  margin: 0;
  font-size: 14px;
  color: #374151;
`;

/* botões de simulação (bem simples) */
export const SimulateRow = styled.div`
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
`;

export const SimButton = styled.button`
  border-radius: 999px;
  border: 1px solid #d1d5db;
  padding: 6px 12px;
  font-size: 12px;
  background: #ffffff;
  color: #4b5563;
  cursor: pointer;
  font-family: inherit;
  transition: background 0.12s ease, border-color 0.12s ease;

  &:hover {
    background: #e5e7eb;
    border-color: #9ca3af;
  }
`;

