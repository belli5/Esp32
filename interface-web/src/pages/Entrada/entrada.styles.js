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
  position: relative;
  width: 100%;
  max-width: 900px;
  background: #ffffff;
  border-radius: 24px;
  padding: 32px 32px 40px;
  box-shadow: 0 18px 45px rgba(15, 23, 42, 0.12);
`;


/* --------- Tabs iguais das outras páginas --------- */

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

export const StepsGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
  margin-bottom: 24px;

  @media (max-width: 900px) {
    grid-template-columns: 1fr;
  }
`;

export const StepCard = styled.div`
  border-radius: 18px;
  padding: 18px 18px 16px;
  background: ${({ disabled }) => (disabled ? "#f3f4f6" : "#f9fafb")};
  border: 1px solid ${({ disabled }) => (disabled ? "#e5e7eb" : "#e5e7eb")};
  opacity: ${({ disabled }) => (disabled ? 0.7 : 1)};
  box-shadow: 0 4px 12px rgba(15, 23, 42, 0.05);
`;

export const StepHeader = styled.span`
  display: inline-block;
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: #6b7280;
  margin-bottom: 4px;
`;

export const StepTitle = styled.h2`
  font-size: 18px;
  margin: 0 0 4px;
  color: #111827;
`;

export const StepDescription = styled.p`
  font-size: 13px;
  margin: 0 0 10px;
  color: #6b7280;
`;

export const StepStatusRow = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 10px;
`;

/* LED reaproveitando lógica de status */

export const LedDot = styled.div`
  width: 16px;
  height: 16px;
  border-radius: 999px;
  background: ${({ status }) => {
    if (status === "success") return "#22c55e"; // verde
    if (status === "error") return "#ef4444"; // vermelho
    if (status === "waiting") return "#facc15"; // amarelo
    return "#9ca3af"; // cinza
  }};
  box-shadow: ${({ status }) =>
    status === "idle" ? "none" : "0 0 10px rgba(0,0,0,0.15)"};
`;

export const StepStatusText = styled.p`
  margin: 0;
  font-size: 13px;
  color: #374151;
`;

export const StepButtonsRow = styled.div`
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
`;

export const StepButton = styled.button`
  border-radius: 999px;
  border: 1px solid ${({ variant }) =>
    variant === "secondary" ? "#d1d5db" : "#2563eb"};
  padding: 6px 12px;
  font-size: 12px;
  background: ${({ variant }) =>
    variant === "secondary" ? "#ffffff" : "#2563eb"};
  color: ${({ variant }) =>
    variant === "secondary" ? "#374151" : "#ffffff"};
  cursor: pointer;
  font-family: inherit;
  transition: background 0.12s ease, border-color 0.12s ease,
    opacity 0.12s ease;

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
  }

  &:hover:not(:disabled) {
    background: ${({ variant }) =>
      variant === "secondary" ? "#f3f4f6" : "#1d4ed8"};
    border-color: ${({ variant }) =>
      variant === "secondary" ? "#9ca3af" : "#1d4ed8"};
  }
`;

/* --------- Status geral --------- */

export const GlobalStatusArea = styled.div`
  margin-top: 8px;
  border-radius: 18px;
  padding: 14px 18px;
  background: #f9fafb;
  border: 1px solid #e5e7eb;
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
`;

export const GlobalStatusBadge = styled.span`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 4px 10px;
  border-radius: 999px;
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: ${({ status }) => (status === "success" ? "#166534" : "#92400e")};
  background: ${({ status }) =>
    status === "success" ? "#bbf7d0" : "#fef3c7"};
`;

export const GlobalStatusText = styled.p`
  margin: 0;
  font-size: 13px;
  color: #374151;
`;

export const BackButton = styled.button`
  position: absolute;
  top: 24px;
  left: 24px;

  border-radius: 999px;
  border: 1px solid #d1d5db;
  padding: 6px 12px;
  font-size: 13px;
  background: #ffffff;
  color: #4b5563;
  cursor: pointer;
  font-family: inherit;
  transition: background 0.12s ease, border-color 0.12s ease;

  &:hover {
    background: #f3f4f6;
    border-color: #9ca3af;
  }
`;