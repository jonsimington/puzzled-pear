#include "action.hpp"

void Action::execute()
{
    m_piece->apply(m_space.rank, m_space.file);
}

std::vector<Action> BoardModel::available_actions()
{
    std::vector<Action> actions;

    for(auto& piece : m_player_pieces)
    {
        switch(piece->type)
        {
            case "Pawn":
                Space space_ahead = piece.location + player.forward;
                if(is_clear(space_ahead))
                {
                    actions.push_back(Action(piece, space_ahead));
                }

                Space attackable_spaces[] = {piece.location + player.forward + one_file,
                                             piece.location + player.forward - one_file}
                for (int i = 0; i < 2; i++) {
                    if(has_opponent_piece(attackable_spaces[i]))
                    {
                        actions.push_back(Action(piece, attackable_spaces[i]));
                    }
                }

                // En passant

                // Promotion
                break;
            default:
            std::cout << "Warning: " << piece->type << " moves not yet implemented." << endl;
        }

        // Pawns go forward 2 on first move

        // Castling
        /*
        if(piece->type == "King")
        {
            // If king is in original position and hasn't moved
            // If rook is in original position and hasn't moved
            // If spaces between king and rook are clear
        }
         */

        // En Passant
    }

    return actions;
}

