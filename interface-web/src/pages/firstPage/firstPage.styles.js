// src/firstPage.styles.js
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


/* ---------- NOVO: barra de abas ---------- */

export const TabsBar = styled.div`
  margin-bottom: 32px;
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

/* ---------- resto que você já tinha ---------- */

export const Title = styled.h1`
  font-size: 40px;
  font-weight: 800;
  color: #020617;
  margin: 0 0 8px;
`;

export const Subtitle = styled.p`
  font-size: 18px;
  color: #6b7280;
  margin: 0 0 40px;
`;

export const ActionsRow = styled.div`
  display: flex;
  gap: 24px;
  justify-content: center;
  flex-wrap: wrap;
`;

export const ActionButton = styled.button`
  flex: 1;
  min-width: 260px;
  max-width: 360px;
  border: none;
  border-radius: 18px;
  padding: 32px 24px;
  cursor: pointer;

  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 12px;

  background: ${({ variant }) =>
    variant === "entrada" ? "#04c35c" : "#0b5cff"};
  color: #ffffff;

  font-family: inherit;
  transition: transform 0.15s ease, box-shadow 0.15s ease, filter 0.15s ease;

  box-shadow: 0 10px 25px
    ${({ variant }) =>
      variant === "entrada"
        ? "rgba(4, 195, 92, 0.35)"
        : "rgba(11, 92, 255, 0.35)"};

  &:hover {
    transform: translateY(-2px);
    filter: brightness(1.03);
  }

  &:active {
    transform: translateY(0);
    filter: brightness(0.97);
    box-shadow: 0 6px 18px rgba(15, 23, 42, 0.2);
  }
`;

export const ActionIcon = styled.span`
  font-size: 24px;
`;

export const ActionLabel = styled.span`
  font-size: 18px;
  font-weight: 700;
  letter-spacing: 0.08em;
`;
